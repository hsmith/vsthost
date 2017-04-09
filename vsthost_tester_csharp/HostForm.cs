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
        private List<PluginForm> plugin_forms = new List<PluginForm>();
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
            int sel = listBox_plugins.SelectedIndex;
            return Convert.ToUInt32(sel >= 0 ? sel : 0);
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
            if (count > 0 && hcp.HasEditor(GetSelectedU()))
            {
                button_show.Enabled = true;
                if (plugin_forms[idx].Visible == true)
                    button_show.Text = "Hide";
                else
                    button_show.Text = "Show";
            }
            else
            {
                button_show.Text = "Show";
                button_show.Enabled = false;
            }
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
                uint last = hcp.GetPluginCount() - 1;
                // to poniżej w OnPluginAdded zwracajacym indeks, chyba rozwiaze to problem wspolbieznosci hmm?
                plugin_forms.Add(new PluginForm(hcp, last));
                //hcp.CreateEditor(last, pf.Handle);
                //listBox_plugins.Items.Add(hcp.GetPluginName(hcp.GetPluginCount() - 1));
                SetSelection(last);
            }
        }

        private void button_delete_Click(object sender, EventArgs e)
        {
            uint selected = GetSelectedU();
            if (selected < hcp.GetPluginCount())
            {
                int selected_s = GetSelected(); // this is v. annoying
                if (plugin_forms[selected_s].Visible == true)
                {
                    plugin_forms[selected_s].CloseEditor();
                }
                hcp.DeletePlugin(selected);
                plugin_forms.RemoveAt(selected_s);
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
                int selected_s = GetSelected(); // this is v. annoying
                PluginForm tmp = plugin_forms[selected_s];
                plugin_forms[selected_s] = plugin_forms[selected_s - 1];
                plugin_forms[selected_s - 1] = tmp;
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
                int selected_s = GetSelected(); // this is v. annoying
                PluginForm tmp = plugin_forms[selected_s];
                plugin_forms[selected_s] = plugin_forms[selected_s + 1];
                plugin_forms[selected_s + 1] = tmp;
                PopulatePluginList();
                SetSelection(selected + 1);
            }
        }

        private void button_show_Click(object sender, EventArgs e)
        {
            //hcp.CreateEditor(0, pf.Handle);
            int sel = GetSelected();
            /*
            uint usel = GetSelectedU();
            hcp.ShowEditor(usel);
            uint width, height;
            if ((width = hcp.GetPluginEditorWidth(usel)) != plugin_forms[sel].Width)
                plugin_forms[sel].Width = Convert.ToInt32(width);
            if ((height = hcp.GetPluginEditorHeight(usel)) != plugin_forms[sel].Height)
                plugin_forms[sel].Height = Convert.ToInt32(height);
            */
            if (plugin_forms[sel].Visible == true)
            {
                button_show.Text = "Show";
                plugin_forms[sel].CloseEditor();
            }
            else
            {
                button_show.Text = "Hide";
                plugin_forms[sel].ShowEditor();
            }
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
