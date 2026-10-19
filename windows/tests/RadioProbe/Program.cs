// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
using SDRSharp.Radio;
namespace SDRSharp.AdvancedDx;
internal unsafe sealed class DecoderBridge
{
    internal readonly List<Complex> Output = [];
    internal void EnqueueIq(CapturePreset preset, Complex* input, int count, double rate, long end, long sequence)
    {
        if (rate != 12000) throw new Exception("Incorrect output rate");
        for (int i=0;i<count;i++) Output.Add(input[i]);
    }
}
internal static unsafe class Program
{
    static void Require(bool value,string error) { if (!value) throw new Exception(error); }
    static (double Hz,double Gain) Tone(double rate,string mode,double audio)
    {
        const long center=14000000,dial=center+8000;
        using var channel=new CaptureChannel(new(dial,mode,15000,0),rate,center);
        var bridge=new DecoderBridge();
        var input=new Complex[8192];
        long n=0;
        for(int block=0;block<Math.Ceiling(rate*.5/input.Length);block++)
        {
            for(int i=0;i<input.Length;i++,n++) { double phase=2*Math.PI*(dial-center+audio)*n/rate; input[i].Real=(float)(.5*Math.Cos(phase));input[i].Imag=(float)(.5*Math.Sin(phase)); }
            var first=input[0];
            fixed(Complex* p=input) channel.Process(p,input.Length,100000,rate,bridge);
            Require(input[0].Real==first.Real&&input[0].Imag==first.Imag,"Host IQ buffer was modified");
        }
        Require(bridge.Output.Count>4000,"Not enough resampled data");
        double crossReal=0,crossImag=0,power=0;
        for(int i=2001;i<bridge.Output.Count;i++)
        {
            var a=bridge.Output[i-1];var b=bridge.Output[i];
            crossReal+=a.Real*b.Real+a.Imag*b.Imag;crossImag+=a.Real*b.Imag-a.Imag*b.Real;
            power+=b.Real*b.Real+b.Imag*b.Imag;
        }
        return(Math.Atan2(crossImag,crossReal)*12000/(2*Math.PI),Math.Sqrt(power/(bridge.Output.Count-2001))/.5);
    }
    static void Main()
    {
        foreach(var rate in new[]{192000d,256000d,2048000d,2400000d})
        foreach(var audio in new[]{200d,1500d,2900d})
        {
            var result=Tone(rate,"FT8",audio);
            Console.WriteLine($"rate={rate} tone={audio} baseband={result.Hz:F3} gain={result.Gain:F5}");
            Require(Math.Abs(result.Hz-(audio-1500))<1,"Frequency shift/sign incorrect");
            Require(result.Gain>.89&&result.Gain<1.12,"USB passband gain exceeds 1 dB tolerance");
        }
        var image=Tone(192000,"FT8",-1000);
        Console.WriteLine($"LSB image gain={image.Gain:F7}");Require(image.Gain<.001,"Opposite sideband not rejected");
        foreach(var audio in new[]{200d,1500d,4500d})
        {
            var result=Tone(256000,"JS8",audio);
            Require(Math.Abs(result.Hz-(audio-3000))<1&&result.Gain>.89&&result.Gain<1.12,"Wide-mode conversion failed");
        }
        Console.WriteLine("SDR# native capture DSP checks passed");
    }
}
