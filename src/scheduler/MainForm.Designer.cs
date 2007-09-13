namespace TaskScheduler
{
    partial class MainForm
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
        	System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
        	this.radDaily = new System.Windows.Forms.RadioButton();
        	this.radWeekly = new System.Windows.Forms.RadioButton();
        	this.timePicker = new System.Windows.Forms.DateTimePicker();
        	this.chkMonday = new System.Windows.Forms.CheckBox();
        	this.chkSaturday = new System.Windows.Forms.CheckBox();
        	this.chkFriday = new System.Windows.Forms.CheckBox();
        	this.chkThursday = new System.Windows.Forms.CheckBox();
        	this.chkWednesday = new System.Windows.Forms.CheckBox();
        	this.chkTuesday = new System.Windows.Forms.CheckBox();
        	this.chkSunday = new System.Windows.Forms.CheckBox();
        	this.cmdCancel = new System.Windows.Forms.Button();
        	this.cmdOk = new System.Windows.Forms.Button();
        	this.cmbDrives = new System.Windows.Forms.ComboBox();
        	this.SuspendLayout();
        	// 
        	// radDaily
        	// 
        	this.radDaily.AutoSize = true;
        	this.radDaily.Location = new System.Drawing.Point(17, 16);
        	this.radDaily.Margin = new System.Windows.Forms.Padding(4);
        	this.radDaily.Name = "radDaily";
        	this.radDaily.Size = new System.Drawing.Size(57, 21);
        	this.radDaily.TabIndex = 0;
        	this.radDaily.TabStop = true;
        	this.radDaily.Text = "Daily";
        	this.radDaily.UseVisualStyleBackColor = true;
        	this.radDaily.CheckedChanged += new System.EventHandler(this.radDaily_CheckedChanged);
        	// 
        	// radWeekly
        	// 
        	this.radWeekly.AutoSize = true;
        	this.radWeekly.Location = new System.Drawing.Point(16, 44);
        	this.radWeekly.Margin = new System.Windows.Forms.Padding(4);
        	this.radWeekly.Name = "radWeekly";
        	this.radWeekly.Size = new System.Drawing.Size(72, 21);
        	this.radWeekly.TabIndex = 1;
        	this.radWeekly.TabStop = true;
        	this.radWeekly.Text = "Weekly";
        	this.radWeekly.UseVisualStyleBackColor = true;
        	this.radWeekly.CheckedChanged += new System.EventHandler(this.radWeekly_CheckedChanged);
        	// 
        	// timePicker
        	// 
        	this.timePicker.Format = System.Windows.Forms.DateTimePickerFormat.Time;
        	this.timePicker.Location = new System.Drawing.Point(169, 14);
        	this.timePicker.Margin = new System.Windows.Forms.Padding(4);
        	this.timePicker.Name = "timePicker";
        	this.timePicker.ShowUpDown = true;
        	this.timePicker.Size = new System.Drawing.Size(143, 22);
        	this.timePicker.TabIndex = 2;
        	this.timePicker.Value = new System.DateTime(2007, 9, 12, 0, 0, 0, 0);
        	// 
        	// chkMonday
        	// 
        	this.chkMonday.AutoSize = true;
        	this.chkMonday.Location = new System.Drawing.Point(12, 74);
        	this.chkMonday.Margin = new System.Windows.Forms.Padding(4);
        	this.chkMonday.Name = "chkMonday";
        	this.chkMonday.Size = new System.Drawing.Size(77, 21);
        	this.chkMonday.TabIndex = 3;
        	this.chkMonday.Text = "&Monday";
        	this.chkMonday.UseVisualStyleBackColor = true;
        	// 
        	// chkSaturday
        	// 
        	this.chkSaturday.AutoSize = true;
        	this.chkSaturday.Location = new System.Drawing.Point(203, 102);
        	this.chkSaturday.Margin = new System.Windows.Forms.Padding(4);
        	this.chkSaturday.Name = "chkSaturday";
        	this.chkSaturday.Size = new System.Drawing.Size(84, 21);
        	this.chkSaturday.TabIndex = 4;
        	this.chkSaturday.Text = "&Saturday";
        	this.chkSaturday.UseVisualStyleBackColor = true;
        	// 
        	// chkFriday
        	// 
        	this.chkFriday.AutoSize = true;
        	this.chkFriday.Location = new System.Drawing.Point(109, 102);
        	this.chkFriday.Margin = new System.Windows.Forms.Padding(4);
        	this.chkFriday.Name = "chkFriday";
        	this.chkFriday.Size = new System.Drawing.Size(66, 21);
        	this.chkFriday.TabIndex = 5;
        	this.chkFriday.Text = "&Friday";
        	this.chkFriday.UseVisualStyleBackColor = true;
        	// 
        	// chkThursday
        	// 
        	this.chkThursday.AutoSize = true;
        	this.chkThursday.Location = new System.Drawing.Point(12, 102);
        	this.chkThursday.Margin = new System.Windows.Forms.Padding(4);
        	this.chkThursday.Name = "chkThursday";
        	this.chkThursday.Size = new System.Drawing.Size(87, 21);
        	this.chkThursday.TabIndex = 6;
        	this.chkThursday.Text = "T&hursday";
        	this.chkThursday.UseVisualStyleBackColor = true;
        	// 
        	// chkWednesday
        	// 
        	this.chkWednesday.AutoSize = true;
        	this.chkWednesday.Location = new System.Drawing.Point(203, 74);
        	this.chkWednesday.Margin = new System.Windows.Forms.Padding(4);
        	this.chkWednesday.Name = "chkWednesday";
        	this.chkWednesday.Size = new System.Drawing.Size(102, 21);
        	this.chkWednesday.TabIndex = 7;
        	this.chkWednesday.Text = "&Wednesday";
        	this.chkWednesday.UseVisualStyleBackColor = true;
        	// 
        	// chkTuesday
        	// 
        	this.chkTuesday.AutoSize = true;
        	this.chkTuesday.Location = new System.Drawing.Point(109, 74);
        	this.chkTuesday.Margin = new System.Windows.Forms.Padding(4);
        	this.chkTuesday.Name = "chkTuesday";
        	this.chkTuesday.Size = new System.Drawing.Size(82, 21);
        	this.chkTuesday.TabIndex = 8;
        	this.chkTuesday.Text = "&Tuesday";
        	this.chkTuesday.UseVisualStyleBackColor = true;
        	// 
        	// chkSunday
        	// 
        	this.chkSunday.AutoSize = true;
        	this.chkSunday.Location = new System.Drawing.Point(12, 130);
        	this.chkSunday.Margin = new System.Windows.Forms.Padding(4);
        	this.chkSunday.Name = "chkSunday";
        	this.chkSunday.Size = new System.Drawing.Size(75, 21);
        	this.chkSunday.TabIndex = 9;
        	this.chkSunday.Text = "S&unday";
        	this.chkSunday.UseVisualStyleBackColor = true;
        	// 
        	// cmdCancel
        	// 
        	this.cmdCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        	this.cmdCancel.Location = new System.Drawing.Point(212, 130);
        	this.cmdCancel.Margin = new System.Windows.Forms.Padding(4);
        	this.cmdCancel.Name = "cmdCancel";
        	this.cmdCancel.Size = new System.Drawing.Size(100, 28);
        	this.cmdCancel.TabIndex = 10;
        	this.cmdCancel.Text = "&Cancel";
        	this.cmdCancel.UseVisualStyleBackColor = true;
        	this.cmdCancel.Click += new System.EventHandler(this.cmdCancel_Click);
        	// 
        	// cmdOk
        	// 
        	this.cmdOk.Location = new System.Drawing.Point(104, 130);
        	this.cmdOk.Margin = new System.Windows.Forms.Padding(4);
        	this.cmdOk.Name = "cmdOk";
        	this.cmdOk.Size = new System.Drawing.Size(100, 28);
        	this.cmdOk.TabIndex = 11;
        	this.cmdOk.Text = "&Ok";
        	this.cmdOk.UseVisualStyleBackColor = true;
        	this.cmdOk.Click += new System.EventHandler(this.cmdOk_Click);
        	// 
        	// cmbDrives
        	// 
        	this.cmbDrives.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        	this.cmbDrives.FormattingEnabled = true;
        	this.cmbDrives.Location = new System.Drawing.Point(169, 44);
        	this.cmbDrives.Name = "cmbDrives";
        	this.cmbDrives.Size = new System.Drawing.Size(143, 24);
        	this.cmbDrives.TabIndex = 12;
        	// 
        	// MainForm
        	// 
        	this.AcceptButton = this.cmdOk;
        	this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
        	this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
        	this.CancelButton = this.cmdCancel;
        	this.ClientSize = new System.Drawing.Size(323, 161);
        	this.Controls.Add(this.cmbDrives);
        	this.Controls.Add(this.cmdOk);
        	this.Controls.Add(this.cmdCancel);
        	this.Controls.Add(this.chkSunday);
        	this.Controls.Add(this.chkTuesday);
        	this.Controls.Add(this.chkWednesday);
        	this.Controls.Add(this.chkThursday);
        	this.Controls.Add(this.chkFriday);
        	this.Controls.Add(this.chkSaturday);
        	this.Controls.Add(this.chkMonday);
        	this.Controls.Add(this.timePicker);
        	this.Controls.Add(this.radWeekly);
        	this.Controls.Add(this.radDaily);
        	this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
        	this.Margin = new System.Windows.Forms.Padding(4);
        	this.Name = "MainForm";
        	this.Text = "UltraDefrag Scheduler";
        	this.ResumeLayout(false);
        	this.PerformLayout();
        }

        #endregion

        private System.Windows.Forms.RadioButton radDaily;
        private System.Windows.Forms.RadioButton radWeekly;
        private System.Windows.Forms.DateTimePicker timePicker;
        private System.Windows.Forms.CheckBox chkMonday;
        private System.Windows.Forms.CheckBox chkSaturday;
        private System.Windows.Forms.CheckBox chkFriday;
        private System.Windows.Forms.CheckBox chkThursday;
        private System.Windows.Forms.CheckBox chkWednesday;
        private System.Windows.Forms.CheckBox chkTuesday;
        private System.Windows.Forms.CheckBox chkSunday;
        private System.Windows.Forms.Button cmdCancel;
        private System.Windows.Forms.Button cmdOk;
        private System.Windows.Forms.ComboBox cmbDrives;
    }
}

