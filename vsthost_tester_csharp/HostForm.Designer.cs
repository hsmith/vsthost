namespace vsthost_tester_csharp
{
    partial class HostForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.listBox_plugins = new System.Windows.Forms.ListBox();
            this.button_add = new System.Windows.Forms.Button();
            this.button_delete = new System.Windows.Forms.Button();
            this.button_up = new System.Windows.Forms.Button();
            this.button_down = new System.Windows.Forms.Button();
            this.button_show = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // listBox_plugins
            // 
            this.listBox_plugins.FormattingEnabled = true;
            this.listBox_plugins.Location = new System.Drawing.Point(12, 12);
            this.listBox_plugins.Name = "listBox_plugins";
            this.listBox_plugins.Size = new System.Drawing.Size(179, 147);
            this.listBox_plugins.TabIndex = 0;
            this.listBox_plugins.SelectedIndexChanged += new System.EventHandler(this.listBox_plugins_SelectedIndexChanged);
            // 
            // button_add
            // 
            this.button_add.Location = new System.Drawing.Point(197, 12);
            this.button_add.Name = "button_add";
            this.button_add.Size = new System.Drawing.Size(98, 23);
            this.button_add.TabIndex = 1;
            this.button_add.Text = "Add";
            this.button_add.UseVisualStyleBackColor = true;
            this.button_add.Click += new System.EventHandler(this.button_add_Click);
            // 
            // button_delete
            // 
            this.button_delete.Location = new System.Drawing.Point(197, 41);
            this.button_delete.Name = "button_delete";
            this.button_delete.Size = new System.Drawing.Size(98, 23);
            this.button_delete.TabIndex = 2;
            this.button_delete.Text = "Delete";
            this.button_delete.UseVisualStyleBackColor = true;
            this.button_delete.Click += new System.EventHandler(this.button_delete_Click);
            // 
            // button_up
            // 
            this.button_up.Location = new System.Drawing.Point(197, 70);
            this.button_up.Name = "button_up";
            this.button_up.Size = new System.Drawing.Size(98, 23);
            this.button_up.TabIndex = 3;
            this.button_up.Text = "Move Up";
            this.button_up.UseVisualStyleBackColor = true;
            this.button_up.Click += new System.EventHandler(this.button_up_Click);
            // 
            // button_down
            // 
            this.button_down.Location = new System.Drawing.Point(197, 99);
            this.button_down.Name = "button_down";
            this.button_down.Size = new System.Drawing.Size(98, 23);
            this.button_down.TabIndex = 4;
            this.button_down.Text = "Move Down";
            this.button_down.UseVisualStyleBackColor = true;
            this.button_down.Click += new System.EventHandler(this.button_down_Click);
            // 
            // button_show
            // 
            this.button_show.Enabled = false;
            this.button_show.Location = new System.Drawing.Point(197, 128);
            this.button_show.Name = "button_show";
            this.button_show.Size = new System.Drawing.Size(98, 23);
            this.button_show.TabIndex = 5;
            this.button_show.Text = "Show";
            this.button_show.UseVisualStyleBackColor = true;
            this.button_show.Click += new System.EventHandler(this.button_show_Click);
            // 
            // HostForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(309, 170);
            this.Controls.Add(this.button_show);
            this.Controls.Add(this.button_down);
            this.Controls.Add(this.button_up);
            this.Controls.Add(this.button_delete);
            this.Controls.Add(this.button_add);
            this.Controls.Add(this.listBox_plugins);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.MaximizeBox = false;
            this.Name = "HostForm";
            this.Text = "Host Controller";
            this.Load += new System.EventHandler(this.HostForm_Load);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListBox listBox_plugins;
        private System.Windows.Forms.Button button_add;
        private System.Windows.Forms.Button button_delete;
        private System.Windows.Forms.Button button_up;
        private System.Windows.Forms.Button button_down;
        private System.Windows.Forms.Button button_show;
    }
}