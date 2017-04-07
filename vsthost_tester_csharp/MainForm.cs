using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace vsthost_tester_csharp
{
    public partial class MainForm : Form
    {
        private Tester tester = new Tester();
        private HostForm hf;

        public MainForm()
        {
            InitializeComponent();

            tester.Initialize();
            hf = new HostForm(tester.GetController());
        }
        
        private void buttonPlay_Click(object sender, EventArgs e)
        {
            tester.Play();
        }

        private void buttonProcess_Click(object sender, EventArgs e)
        {
            tester.Process();
        }

        private void buttonStop_Click(object sender, EventArgs e)
        {
            tester.Stop();
        }

        private void buttonHost_Click(object sender, EventArgs e)
        {
            hf.Show();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {

        }
    }
}
