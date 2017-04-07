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
    public partial class HostForm : Form
    {
        private OpenFileDialog ofd = new OpenFileDialog();
        private HostControllerProxy hcp;
        public HostForm(HostControllerProxy hcp_)
        {
            hcp = hcp_;
            InitializeComponent();
            SetButtons();
        }

        private int GetSelected()
        {
            return listBox_plugins.SelectedIndex;
        }

        private uint GetSelectedU()
        {
            return Convert.ToUInt32(GetSelected());
        }

        private void PopulatePluginList()
        {
            listBox_plugins.Items.Clear();
            for (uint i = 0; i < hcp.GetPluginCount(); ++i)
            {
                listBox_plugins.Items.Add(hcp.GetPluginName(i));
            }
        }

        private void SetButtons()
        {
            int count = listBox_plugins.Items.Count;
            int idx = GetSelected();
            button_up.Enabled = count >= 2 && idx > 0;
            button_down.Enabled = count >= 2 && idx < count - 1;
            button_delete.Enabled = count > 0;
        }

        private void SetSelection(uint idx)
        {
            listBox_plugins.SetSelected(Convert.ToInt32(idx), true);
            listBox_plugins.Focus();
            SetButtons();
        }

        private void button_add_Click(object sender, EventArgs e)
        {
            if (ofd.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                hcp.AddPlugin(ofd.FileName);
                // need to know wheter its been added
                PopulatePluginList();
                SetSelection(hcp.GetPluginCount() - 1);
                //listBox_plugins.Items.Add(hcp.GetPluginName(hcp.GetPluginCount() - 1));
            }
        }

        private void button_delete_Click(object sender, EventArgs e)
        {
            if (GetSelectedU() < hcp.GetPluginCount())
            {
                hcp.DeletePlugin(GetSelectedU());
                // or deleted
                PopulatePluginList();
                if (hcp.GetPluginCount() > 0)
                    SetSelection(hcp.GetPluginCount() - 1);
                else
                    SetButtons();
                //listBox_plugins.Items.RemoveAt(GetSelected());
            }
        }

        private void button_up_Click(object sender, EventArgs e)
        {
            uint selected = GetSelectedU();
            if (selected < hcp.GetPluginCount() && selected > 0)
            {
                hcp.MoveUp(selected);
                PopulatePluginList();
                SetSelection(selected - 1);
            }
        }

        private void button_down_Click(object sender, EventArgs e)
        {
            uint selected = GetSelectedU();
            if (selected < hcp.GetPluginCount() - 1)
            {
                hcp.MoveDown(selected);
                PopulatePluginList();
                SetSelection(selected + 1);
            }
        }

        private void button_show_Click(object sender, EventArgs e)
        {

        }

        private void listBox_plugins_SelectedIndexChanged(object sender, EventArgs e)
        {
            SetButtons();
        }

        private void HostForm_Load(object sender, EventArgs e)
        {

        }
    }
}
