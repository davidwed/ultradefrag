/*
 *  UltraDefragScheduler.NET
 *  Copyright (c) 2007 by:
 *		Justin Dearing (zippy1981@gmail.com)
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace UltraDefrag.Scheduler
{
    /// <summary>
    /// Form for entering scheduling information for the task.
    /// </summary>
    public partial class MainForm : Form
    {
        /// <summary>
        /// Default constructor. Initializes the form.
        /// </summary>
        public MainForm()
        {
            InitializeComponent();
            //TODO: filter the list for only local writeable drives.
            DriveInfo[] drives = DriveInfo.GetDrives();
            cmbDrives.DataSource = drives;
            cmbDrives.DisplayMember = "Name";
            cmbDrives.ValueMember = "Name";
        }

        #region Event Handlers

        private void radDaily_CheckedChanged(object sender, EventArgs e)
        {
            chkMonday.Enabled = false;
            chkTuesday.Enabled = false;
            chkWednesday.Enabled = false;
            chkThursday.Enabled = false;
            chkFriday.Enabled = false;
            chkSaturday.Enabled = false;
            chkSunday.Enabled = false;
        }


        private void radWeekly_CheckedChanged(object sender, EventArgs e)
        {
            chkMonday.Enabled = true;
            chkTuesday.Enabled = true;
            chkWednesday.Enabled = true;
            chkThursday.Enabled = true;
            chkFriday.Enabled = true;
            chkSaturday.Enabled = true;
            chkSunday.Enabled = true;
        }

        private void cmdCancel_Click(object sender, EventArgs e)
        {
            Close();
        }

        private void cmdOk_Click(object sender, EventArgs e)
        {
            IntPtr ptr;
            AT_INFO atInfo = new AT_INFO();
            atInfo.Command = string.Format("udefrag {0}", cmbDrives.SelectedValue);
            atInfo.DaysOfMonth = 0;
            atInfo.DaysOfWeek = 0;
            atInfo.Flags = AT_JobFlags.JOB_RUN_PERIODICALLY | AT_JobFlags.JOB_NONINTERACTIVE;
            atInfo.JobTime = (uint) timePicker.Value.TimeOfDay.TotalMilliseconds;
            int jobId;

            if (radDaily.Checked)
            {
                atInfo.DaysOfWeek = 0x7F;
            }
            else if (radWeekly.Checked)
            {
                if (chkMonday.Checked) atInfo.DaysOfWeek |= 0x1;
                if (chkTuesday.Checked) atInfo.DaysOfWeek |= 0x2;
                if (chkWednesday.Checked) atInfo.DaysOfWeek |= 0x4;
                if (chkThursday.Checked) atInfo.DaysOfWeek |= 0x8;
                if (chkFriday.Checked) atInfo.DaysOfWeek |= 0x10;
                if (chkSaturday.Checked) atInfo.DaysOfWeek |= 0x20;
                if (chkSunday.Checked) atInfo.DaysOfWeek |= 0x40;
            }

            ptr = Marshal.AllocHGlobal(Marshal.SizeOf(atInfo));
            Marshal.StructureToPtr(atInfo, ptr, true);

            Native.NetScheduleJobAdd(null, ptr, out jobId);
        }

        #endregion Event Handlers
    }
}
