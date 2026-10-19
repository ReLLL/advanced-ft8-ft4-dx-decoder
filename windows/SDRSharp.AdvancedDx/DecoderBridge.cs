// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
using System.Buffers.Binary;
using System.Buffers;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text;
using System.Text.Json;
using SDRSharp.Common;
using SDRSharp.Radio;

namespace SDRSharp.AdvancedDx;

internal unsafe sealed class DecoderBridge : IIQProcessor, IStreamProcessor, IDisposable
{
    private readonly ISharpControl control;
    private readonly CornerOverlay overlay;
    private readonly object captureLock = new();
    private readonly record struct Packet(byte[] Data, int Length, bool Pooled = false);
    private BlockingCollection<Packet> outgoing = new(24);
    private CancellationTokenSource stopping = new();
    private readonly List<CaptureChannel> channels = [];
    private readonly System.Windows.Forms.Timer metadataTimer = new() { Interval = 250 };
    private CapturePreset[] presets = [];
    private CapturePreset[] applied = [];
    private Process? worker;
    private Thread? writer, reader;
    private volatile bool disposed, workerEnabled = true, workerRunning;
    private bool captureEnabled = true;
    private double sampleRate, configuredRate;
    private long configuredCenter, skipped, blocks, stations, countries;
    private string error = "Start the radio, then open the decoder.";
    private int restarting, resetNeeded = 1;
    private double streamEndMs;
    public bool Enabled { get => Volatile.Read(ref captureEnabled); set { Volatile.Write(ref captureEnabled, value); Interlocked.Exchange(ref resetNeeded,1); Utils.SaveSetting("advancedDx.enabled",value); SendMetadata(); } }
    public double SampleRate { set => Volatile.Write(ref sampleRate, value); }
    internal string Status => !string.IsNullOrEmpty(error) ? error : workerRunning ? $"{(!Enabled ? "Decoder off" : !workerEnabled ? "Paused" : control.IsPlaying ? "Receiving" : "Radio stopped")}\n{Interlocked.Read(ref stations)} stations | {Interlocked.Read(ref countries)} countries" : "Decoder stopped. Reopen to retry.";
    internal string Diagnostics => $"{Interlocked.Read(ref blocks)} capture blocks; {Interlocked.Read(ref skipped)} skipped blocks. Closing the decoder window keeps reception active.";

    internal DecoderBridge(ISharpControl control)
    {
        this.control = control;
        captureEnabled = Utils.GetBooleanSetting("advancedDx.enabled",true);
        overlay = new CornerOverlay(control);
        metadataTimer.Tick += (_, _) => { if (!disposed) { EnsureWorker(); SendMetadata(); } };
        metadataTimer.Start();
    }

    private void EnsureWorker(bool force=false)
    {
        if (!force && !Enabled && worker == null) return;
        if (disposed || workerRunning || Interlocked.Exchange(ref restarting, 1) != 0) return;
        try
        {
            // Fail closed after a worker exit. A button click requests recovery.
            if (worker != null) return;
            var directory = Path.GetDirectoryName(typeof(AdvancedDxPlugin).Assembly.Location)!;
            var binary = Path.Combine(directory, "AdvancedDx", "dx_sdrsharp.exe");
            if (!File.Exists(binary)) { error = "Decoder files missing. Reinstall the complete SDR# package."; return; }
            var data = Environment.GetEnvironmentVariable("ADVANCED_DX_DATA_ROOT") ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Advanced DX Decoder", "SDRSharp");
            var info = new ProcessStartInfo(binary) { UseShellExecute = false, RedirectStandardInput = true, RedirectStandardOutput = true, RedirectStandardError = true, CreateNoWindow = true, WorkingDirectory = Path.GetDirectoryName(binary)! };
            info.ArgumentList.Add("--data-root"); info.ArgumentList.Add(data);
            info.ArgumentList.Add("--parent"); info.ArgumentList.Add(Environment.ProcessId.ToString());
            worker = System.Diagnostics.Process.Start(info) ?? throw new IOException("Decoder process did not start");
            error = "";
            var process = worker; var queue = outgoing; var token = stopping.Token;
            process.EnableRaisingEvents = true;
            process.Exited += (_, _) => { if (ReferenceEquals(worker, process)) { workerRunning = false; if (!disposed) error = "Decoder stopped. Open it again to retry; saved receptions are kept."; } };
            workerRunning = !process.HasExited;
            process.ErrorDataReceived += (_, e) => { if (ReferenceEquals(worker, process) && !string.IsNullOrEmpty(e.Data)) error = e.Data; };
            process.BeginErrorReadLine();
            writer = new Thread(() => WriteLoop(process, queue, token)) { IsBackground = true, Name = "Kondor capture IPC" };
            reader = new Thread(() => ReadLoop(process, token)) { IsBackground = true, Name = "Kondor status IPC" };
            writer.Start(); reader.Start();
        }
        catch (Exception e) when (e is IOException or System.ComponentModel.Win32Exception) { error = "Decoder could not start: " + e.Message; }
        finally { Interlocked.Exchange(ref restarting, 0); }
    }

    internal void ShowWindow()
    {
        if (disposed) return;
        if (worker != null && !workerRunning && Interlocked.CompareExchange(ref restarting, 1, 0) == 0)
        {
            error = "Restarting decoder; saved receptions are kept.";
            _ = Task.Run(() => { try { lock (captureLock) { if (!disposed) StopSession(); } } finally { Interlocked.Exchange(ref restarting, 0); } });
            return;
        }
        EnsureWorker(true);
        Enqueue(new { kind = "show" });
    }

    private void SendMetadata()
    {
        Enqueue(new { kind = "metadata", center = control.CenterFrequency, frequency = control.Frequency,
            sample_rate = Volatile.Read(ref sampleRate) > 0 ? Volatile.Read(ref sampleRate) : control.InputSampleRate,
            spectrum_width = control.RFDisplayBandwidth,
            playing = control.IsPlaying && Enabled,
            replay = control.SourceIsWaveFile || control.SourceType?.FullName == "SDRSharp.FrontEnds.FilePlayer.WAVEFileIO" });
    }

    public void Process(Complex* input, int count)
    {
        if (disposed || !Enabled || !workerEnabled || !workerRunning || input == null || count <= 0 || count > 1024*1024) return;
        if (!Monitor.TryEnter(captureLock)) { Interlocked.Exchange(ref resetNeeded,1); Interlocked.Increment(ref skipped); return; }
        try
        {
            try
            {
                if (disposed || outgoing.IsAddingCompleted || outgoing.Count > 16) { Interlocked.Exchange(ref resetNeeded,1); Interlocked.Increment(ref skipped); return; }
                var rate = Volatile.Read(ref sampleRate);
                var center = control.CenterFrequency;
                var current = Volatile.Read(ref presets);
                if (rate < 12000 || !double.IsFinite(rate) || !control.IsPlaying) return;
                double now=DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
                bool discontinuity=Interlocked.Exchange(ref resetNeeded,0)!=0 || Math.Abs(now-(streamEndMs+count*1000/rate))>1000;
                if (discontinuity || configuredRate != rate || configuredCenter != center || !ReferenceEquals(applied, current))
                {
                    foreach (var channel in channels) channel.Dispose();
                    channels.Clear();
                    foreach (var preset in current.Where(p => p.Dial >= center-rate/2+6000 && p.Dial+6000 <= center+rate/2).Take(8))
                        channels.Add(new CaptureChannel(preset, rate, center));
                    configuredRate = rate; configuredCenter = center; applied = current;
                    streamEndMs=now-count*1000/rate;
                    Enqueue(new { kind = "reset" });
                }
                streamEndMs+=count*1000/rate;
                var end=(long)Math.Round(streamEndMs);
                foreach (var channel in channels) channel.Process(input, count, end, rate, this);
                Interlocked.Increment(ref blocks);
            }
            catch (Exception e) when (e is ArgumentException or InvalidOperationException or OverflowException)
            { error = "Capture reset: " + e.Message; foreach (var channel in channels) channel.Dispose(); channels.Clear(); configuredRate = 0; Interlocked.Increment(ref skipped); }
        }
        finally { Monitor.Exit(captureLock); }
    }

    internal void EnqueueIq(CapturePreset preset, Complex* input, int count, double rate, long end, long sequence)
    {
        var header = JsonSerializer.SerializeToUtf8Bytes(new { kind = "iq", dial = preset.Dial, mode = preset.Mode, period_ms = preset.PeriodMs, submode = preset.Submode, samples = count, sample_rate = rate, end_ms = end, sequence });
        var payloadSize = checked(count * sizeof(Complex));
        if (payloadSize > 1024*1024) { Interlocked.Increment(ref skipped); return; }
        var length = 4 + header.Length + payloadSize;
        var packet = ArrayPool<byte>.Shared.Rent(length);
        BinaryPrimitives.WriteInt32LittleEndian(packet, header.Length);
        header.CopyTo(packet, 4);
        fixed (byte* destination = &packet[4+header.Length]) Buffer.MemoryCopy(input, destination, payloadSize, payloadSize);
        if (!TryQueue(new Packet(packet, length, true))) { ArrayPool<byte>.Shared.Return(packet); Interlocked.Increment(ref skipped); }
    }

    private void Enqueue<T>(T value)
    {
        if (disposed || stopping.IsCancellationRequested) return;
        var header = JsonSerializer.SerializeToUtf8Bytes(value);
        var packet = new byte[4+header.Length];
        BinaryPrimitives.WriteInt32LittleEndian(packet, header.Length);
        header.CopyTo(packet,4);
        if (!TryQueue(new Packet(packet, packet.Length))) Interlocked.Increment(ref skipped);
    }

    private bool TryQueue(Packet packet)
    {
        try { return outgoing.TryAdd(packet); }
        catch (Exception e) when (e is InvalidOperationException or ObjectDisposedException) { return false; }
    }

    private void WriteLoop(Process process, BlockingCollection<Packet> queue, CancellationToken token)
    {
        try { foreach (var packet in queue.GetConsumingEnumerable(token)) { try { process.StandardInput.BaseStream.Write(packet.Data, 0, packet.Length); process.StandardInput.BaseStream.Flush(); } finally { if (packet.Pooled) ArrayPool<byte>.Shared.Return(packet.Data); } } }
        catch (Exception e) when (e is IOException or OperationCanceledException or ObjectDisposedException) { Disconnected(process,token,"Decoder disconnected. Open it again to retry."); }
    }

    private void ReadLoop(Process process, CancellationToken token)
    {
        try
        {
            var input = process.StandardOutput.BaseStream;
            var length = new byte[4];
            while (!token.IsCancellationRequested)
            {
                input.ReadExactly(length);
                var count = BinaryPrimitives.ReadInt32LittleEndian(length);
                if (count <= 0 || count > 1024*1024) throw new IOException("Invalid decoder configuration");
                var body = new byte[count]; input.ReadExactly(body);
                using var doc = JsonDocument.Parse(body);
                var message = doc.RootElement;
                if (message.GetProperty("kind").GetString() != "plan") continue;
                bool ready=message.GetProperty("enabled").GetBoolean();
                if(ready!=workerEnabled) Interlocked.Exchange(ref resetNeeded,1);
                workerEnabled = ready;
                if (message.TryGetProperty("stations", out var heardStations)) Interlocked.Exchange(ref stations, heardStations.GetInt64());
                if (message.TryGetProperty("countries", out var heardCountries)) Interlocked.Exchange(ref countries, heardCountries.GetInt64());
                if (message.TryGetProperty("corner_rows", out var corner)) overlay.Update(corner);
                var next = message.GetProperty("presets").EnumerateArray().Select(p => new CapturePreset(p.GetProperty("dial").GetInt64(), p.GetProperty("mode").GetString()!, p.GetProperty("period_ms").GetInt32(), p.GetProperty("submode").GetInt32())).Take(32).ToArray();
                var old = Volatile.Read(ref presets);
                if (!next.SequenceEqual(old)) Volatile.Write(ref presets, next);
            }
        }
        catch (Exception e) when (e is IOException or JsonException or KeyNotFoundException or InvalidOperationException or ObjectDisposedException) { Disconnected(process,token,"Decoder status disconnected. Open it again to retry."); }
    }

    private void Disconnected(Process process,CancellationToken token,string reason)
    {
        if(disposed||token.IsCancellationRequested||!ReferenceEquals(worker,process)) return;
        workerRunning=false;Interlocked.Exchange(ref resetNeeded,1);error=reason;
    }

    private void StopSession()
    {
        var process = worker; worker = null; workerRunning = false;
        stopping.Cancel(); outgoing.CompleteAdding();
        try { if (process is { HasExited: false }) { process.StandardInput.Close(); if (!process.WaitForExit(8000)) process.Kill(true); } }
        catch (InvalidOperationException) { }
        foreach (var channel in channels) channel.Dispose(); channels.Clear(); configuredRate = 0; applied = [];
        writer?.Join(1000); reader?.Join(1000); process?.Dispose(); stopping.Dispose();
        while (outgoing.TryTake(out var pending)) if (pending.Pooled) ArrayPool<byte>.Shared.Return(pending.Data);
        outgoing.Dispose();
        worker = null; writer = reader = null; stopping = new CancellationTokenSource(); outgoing = new BlockingCollection<Packet>(24);
    }

    public void Dispose()
    {
        disposed = true; metadataTimer.Dispose(); overlay.Dispose();
        lock (captureLock) { StopSession(); stopping.Dispose(); outgoing.Dispose(); }
    }
}
