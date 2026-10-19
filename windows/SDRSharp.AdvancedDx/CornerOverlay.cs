// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
using System.Diagnostics;
using System.Drawing.Drawing2D;
using System.Text.Json;
using SDRSharp.Common;
using SDRSharp.PanView;

namespace SDRSharp.AdvancedDx;

internal sealed class CornerOverlay : IDisposable
{
    private sealed record Row(string Key, string Country, string Call, string Distance, string Snr, string Message, string Flag, bool Record);
    private readonly ISharpControl control;
    private readonly Font regular = new("Segoe UI", 8.5f);
    private readonly Font italic = new("Segoe UI", 8.5f, FontStyle.Italic);
    private readonly Dictionary<string, Bitmap> images = [];
    private readonly Dictionary<string, (string Key, long Tick)> flashes = [];
    private Row[] rows = [];
    internal CornerOverlay(ISharpControl control) { this.control = control; control.WaterfallCustomPaint += Paint; }
    internal void Update(JsonElement value)
    {
        var next = value.EnumerateArray().Take(76).Select(p => new Row(p.GetProperty("key").GetString()!, p.GetProperty("country").GetString()!, p.GetProperty("call").GetString()!, p.GetProperty("distance").GetString()!, p.GetProperty("snr").GetString()!, p.GetProperty("message").GetString()!, p.GetProperty("flag").GetString()!, p.GetProperty("record").GetBoolean())).ToArray();
        Volatile.Write(ref rows, next);
    }
    private Bitmap? Icon(string code, bool trophy = false)
    {
        var name = trophy ? "1f3c6" : code.Length == 2 && code.All(c => c is >= 'A' and <= 'Z') ? $"{0x1f1e6+code[0]-'A':x}-{0x1f1e6+code[1]-'A':x}" : "1f3f3";
        if (images.TryGetValue(name, out var found)) return found;
        var file = Path.Combine(Path.GetDirectoryName(typeof(AdvancedDxPlugin).Assembly.Location)!, "AdvancedDx", "icons", name+".png");
        if (!File.Exists(file)) return null;
        using var original = Image.FromFile(file);
        var image = new Bitmap(original); images[name] = image; return image;
    }
    private void Paint(object? sender, CustomPaintEventArgs args)
    {
        var current = Volatile.Read(ref rows);
        if (current.Length == 0) { flashes.Clear(); return; }
        var g = args.Graphics;
        var saved = g.Save();
        try
        {
            var clip = g.VisibleClipBounds;
            var line = regular.GetHeight(g) + 6;
            var maximumWidth = Math.Min(900, clip.Width-16);
            if (maximumWidth < 180 || clip.Height < line*2) return;
            var available = Math.Max(1, (int)((clip.Height-16)/line));
            var last = current.LastOrDefault(r => r.Record);
            var visible = current.Where(r => !r.Record).Take(available-(last == null ? 0 : 1)).ToList();
            if (last != null) visible.Add(last);
            float Measure(string text,Font font) => g.MeasureString(text,font,int.MaxValue,StringFormat.GenericTypographic).Width+8;
            var contentWidth=visible.Max(r => 21 + (r.Record ? 21+Measure("Farthest",regular) : 0) +
                Measure(r.Country,regular)+Measure(r.Call,regular)+Measure(r.Distance,regular)+Measure(r.Snr,regular)+Measure('"'+r.Message+'"',italic));
            var width=Math.Min(maximumWidth,Math.Max(180,contentWidth));
            var x = clip.Right-width-8;
            var y = clip.Bottom-line*visible.Count-8;
            using var background = new SolidBrush(Color.FromArgb(195,12,15,18));
            g.FillRectangle(background, x-4,y-2,width+8,line*visible.Count+4);
            g.SetClip(new RectangleF(x,y,width,line*visible.Count), CombineMode.Intersect);
            var seen = new HashSet<string>();
            foreach (var row in visible)
            {
                var identity = row.Record ? "farthest" : row.Key;
                seen.Add(identity);
                if (!flashes.TryGetValue(identity,out var flash) || flash.Key != row.Key) flashes[identity] = flash = (row.Key,Stopwatch.GetTimestamp());
                var age = Stopwatch.GetElapsedTime(flash.Tick).TotalSeconds;
                if (age is > 0 and < 2) { using var pulse = new SolidBrush(Color.FromArgb((int)(45*Math.Sin(age*Math.PI/2)),115,210,140)); g.FillRectangle(pulse,x,y,width,line); }
                var left = x;
                void Image(Bitmap? image) { if (image != null) g.DrawImage(image,left,y+1,16,16); left += 21; }
                void Part(string text, Color color, bool slant=false) { using var brush = new SolidBrush(color); var font = slant ? italic : regular; g.DrawString(text,font,brush,left,y+1,StringFormat.GenericTypographic); left += g.MeasureString(text,font,int.MaxValue,StringFormat.GenericTypographic).Width+8; }
                if (row.Record) Image(Icon("",true));
                Image(Icon(row.Flag));
                if (row.Record) Part("Farthest",Color.FromArgb(255,204,82));
                Part(row.Country,Color.FromArgb(161,227,168)); Part(row.Call,Color.FromArgb(168,209,255)); Part(row.Distance,Color.FromArgb(255,194,140));
                Part(row.Snr,Color.FromArgb(224,230,237)); Part('"'+row.Message+'"',Color.FromArgb(224,230,237),true);
                if (new RectangleF(x,y,width,line).Contains(args.CursorPosition)) args.CustomTitle = $"{row.Country} | {row.Call} | {row.Distance} | {row.Snr} | \"{row.Message}\"";
                y += line;
            }
            foreach (var key in flashes.Keys.Where(k => !seen.Contains(k)).ToArray()) flashes.Remove(key);
        }
        finally { g.Restore(saved); }
    }
    public void Dispose() { control.WaterfallCustomPaint -= Paint; foreach (var image in images.Values) image.Dispose(); images.Clear(); regular.Dispose(); italic.Dispose(); }
}
