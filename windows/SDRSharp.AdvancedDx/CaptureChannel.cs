// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
using System.Runtime.InteropServices;
using SDRSharp.Radio;

namespace SDRSharp.AdvancedDx;

internal sealed record CapturePreset(long Dial, string Mode, int PeriodMs, int Submode);

// SDR#'s native DSP supplies mixing, resampling and USB filtering.
// Only private copies are processed; the host's IQ buffer is never modified.
internal unsafe sealed class CaptureChannel : IDisposable
{
    internal readonly CapturePreset Preset;
    private readonly DownConverter converter;
    private readonly IQResampler resampler;
    private readonly ComplexFilter filter;
    private Complex* buffer;
    private Complex* output;
    private int capacity;
    private int outputCapacity;
    private long sequence;
    internal CaptureChannel(CapturePreset preset, double sampleRate, long center)
    {
        Preset = preset;
        int ratio = 1;
        while (sampleRate / (ratio * 2) >= 24000 && ratio < 4096) ratio *= 2;
        converter = new DownConverter(sampleRate, ratio);
        converter.Frequency = preset.Dial + (preset.Mode is "FT8" or "FT4" ? 1500 : 3000) - center;
        if (converter.DecimationRatio != ratio) { converter.Dispose(); throw new InvalidOperationException("Unsupported SDR# decimation ratio"); }
        try
        {
            resampler = new IQResampler(sampleRate / ratio, 12000);
            if (Math.Abs(resampler.ActualSampleRate - 12000) > .01) throw new InvalidOperationException("Unsupported SDR# output sample rate");
            filter = new ComplexFilter(FilterBuilder.MakeLowPassKernel(12000, 255, preset.Mode is "FT8" or "FT4" ? 1750 : 3000, WindowType.BlackmanHarris4));
        }
        catch { resampler?.Dispose(); converter.Dispose(); throw; }
    }
    internal void Process(Complex* input, int count, long endMs, double sampleRate, DecoderBridge bridge)
    {
        if (count > capacity)
        {
            NativeMemory.Free(buffer);
            buffer = (Complex*)NativeMemory.Alloc(checked((nuint)count * (nuint)sizeof(Complex)));
            capacity = count;
        }
        Buffer.MemoryCopy(input, buffer, (long)capacity * sizeof(Complex), (long)count * sizeof(Complex));
        var outputCount = converter.Process(buffer, count);
        if (outputCount <= 0 || outputCount > count) return;
        int required = checked((int)Math.Ceiling(outputCount * 12000 / (sampleRate / converter.DecimationRatio)) + 4096);
        if (required > outputCapacity)
        {
            NativeMemory.Free(output);
            output = (Complex*)NativeMemory.Alloc(checked((nuint)required * (nuint)sizeof(Complex)));
            outputCapacity = required;
        }
        int resampled = resampler.Process(buffer, output, outputCount);
        if (resampled <= 0 || resampled > outputCapacity) throw new InvalidOperationException("Invalid resampler output length");
        filter.Process(output, resampled);
        bridge.EnqueueIq(Preset, output, resampled, 12000, endMs - (long)Math.Round(filter.Delay * 1000.0 / 12000), sequence++);
    }
    public void Dispose() { filter.Dispose(); resampler.Dispose(); converter.Dispose(); NativeMemory.Free(buffer); NativeMemory.Free(output); buffer = output = null; }
}
