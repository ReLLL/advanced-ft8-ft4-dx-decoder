// Copyright 2026 ReLLL. SPDX-License-Identifier: GPL-3.0-or-later
using SDRSharp.Common;
using SDRSharp.Radio;

namespace SDRSharp.AdvancedDx;

public sealed class AdvancedDxPlugin : ISharpPlugin, ICanLazyLoadGui, ISupportStatus, IExtendedNameProvider
{
    private ISharpControl? control;
    private DecoderBridge? bridge;
    private ControlPanel? panel;
    public string DisplayName => "Advanced FT8/FT4/WSPR DX Decoder";
    public string Category => "Digital";
    public string MenuItemName => DisplayName;
    public bool IsActive => bridge?.Enabled == true;
    public UserControl Gui { get { LoadGui(); return panel!; } }

    public void Initialize(ISharpControl sharpControl)
    {
        control = sharpControl;
        bridge = new DecoderBridge(sharpControl);
        sharpControl.RegisterStreamHook(bridge, ProcessorType.RawIQ);
    }

    public void LoadGui()
    {
        if (panel == null && bridge != null) panel = new ControlPanel(bridge);
    }

    public void Close()
    {
        if (bridge == null) return;
        control?.UnregisterStreamHook(bridge);
        panel?.Dispose();
        bridge.Dispose();
        bridge = null;
    }
}

internal sealed class ControlPanel : UserControl
{
    private readonly System.Windows.Forms.Timer timer = new() { Interval = 1000 };
    private readonly Label status = new() { AutoSize = true, MaximumSize = new Size(300, 0) };
    private readonly ToolTip tip = new();
    internal ControlPanel(DecoderBridge bridge)
    {
        AutoScaleMode = AutoScaleMode.Dpi;
        AutoSize = true;
        var layout = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoSize = true, FlowDirection = FlowDirection.TopDown, WrapContents = false };
        var enabled = new CheckBox { Text = "Enable decoder", Checked = bridge.Enabled, AutoSize = true };
        var open = new Button { Text = "Open Advanced DX Decoder", AutoSize = true };
        enabled.CheckedChanged += (_, _) => bridge.Enabled = enabled.Checked;
        open.Click += (_, _) => bridge.ShowWindow();
        layout.Controls.AddRange([enabled, open, status]);
        Controls.Add(layout);
        timer.Tick += (_, _) => { status.Text = bridge.Status; tip.SetToolTip(status,bridge.Diagnostics); };
        timer.Start();
    }
    protected override void Dispose(bool disposing) { if (disposing) { timer.Dispose(); tip.Dispose(); } base.Dispose(disposing); }
}
