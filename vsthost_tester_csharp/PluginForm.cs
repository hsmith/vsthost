using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using VSTHost;

namespace vsthost_tester_csharp
{
    public partial class PluginForm : Form
    {
        public uint idx { get; set; }
        private HostControllerProxy hcp;
        private MenuItem preset_menu = new MenuItem("Preset");
        private bool size_fixed = false;

        public PluginForm(HostControllerProxy hcp_, uint idx_)
        {
            InitializeComponent();
            hcp = hcp_;
            idx = idx_;
            this.Text = hcp.GetPluginName(idx);
            for (uint i = 0; i < hcp.GetPluginPresetCount(idx); ++i)
                preset_menu.MenuItems.Add(hcp.GetPluginPresetName(idx, i));
            mainMenu1.MenuItems.Add(preset_menu);
            hcp.CreateEditor(idx, Handle);
            FixSize();
        }

        void FixSize()
        {
            uint w = hcp.GetPluginEditorWidth(idx);
            uint h = hcp.GetPluginEditorHeight(idx);
            bool need_to_fix = false;
            if (w != ClientSize.Width)
            {
                need_to_fix = true;
                //Width = Convert.ToInt32(w);
                //Width += System.Windows.Forms.SystemInformation. * 2;
            }
            if (h != ClientSize.Height)
            {
                need_to_fix = true;
                //Height = Convert.ToInt32(h);
                //Height += System.Windows.Forms.SystemInformation.MenuHeight
                //    + System.Windows.Forms.SystemInformation.CaptionHeight
                //    + System.Windows.Forms.SystemInformation.BorderSize.Height * 2;
            }
            if (need_to_fix)
                ClientSize = new Size(Convert.ToInt32(w), Convert.ToInt32(h));
        }

        public void ShowEditor() {
            hcp.ShowEditor(idx);
            if (size_fixed == false)
            {
                FixSize();
                size_fixed = true;
            }
            //MessageBox.Show(Height.ToString() + " " + Width.ToString());
            Show();
        }

        public void CloseEditor()
        {
            hcp.HideEditor(idx);
            Hide();
        }

        private void PluginForm_FormClosing(object sender, FormClosingEventArgs e)
        {
            e.Cancel = true;
            CloseEditor();
        }
    }
}
