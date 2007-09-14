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
using System.Runtime.InteropServices;

// Special thanks to Willy Denoyette for posting the pInvoke code on
// http://www.dotnet247.com/247reference/msgs/47/239675.aspx

namespace UltraDefrag.Scheduler
{
    /// <summary>
    /// Info for a scheduled task.
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    internal struct AT_INFO
    {
        /// <summary>
        /// Time the job is run. Expressed in milliseconds from midnight of the day the
        /// job is to be run.
        /// </summary>
        public uint JobTime;
        /// <summary>
        /// Bitmast of days of the month that the job is to be run.
        /// Bitmask is from the first to thirty first, lowest to highest bit.
        /// </summary>
        public uint DaysOfMonth;
        /// <summary>
        /// Bitmast of days of the week that the job is to be run.
        /// Bitmask is from Monday to Friday, from lowest to highest bit.
        /// </summary>
        public byte DaysOfWeek;
        /// <summary>
        /// Flags to specify info about the jobs.
        /// </summary>
        public AT_JobFlags Flags;
        /// <summary>
        /// The command to run.
        /// </summary>
        [MarshalAs(UnmanagedType.LPTStr)]
        public string Command;
    }

    /// <remarks />
    internal enum AT_JobFlags : byte
    {
        JOB_RUN_PERIODICALLY = 1,
        JOB_EXEC_ERROR = 2,
        JOB_RUNS_TODAY = 4,
        JOB_ADD_CURRENT_DATE = 8,
        JOB_NONINTERACTIVE = 16
    };

    /// <summary>
    /// Native functions accessed via pInvoke.
    /// </summary>
    internal class Native
    {
        /// <summary>
        /// Adds a windows schedukled task.
        /// </summary>
        /// <param name="Servername">
        /// The name of the machine to add the scheduled task to.
        /// NULL for localhost.
        /// </param>
        /// <param name="Buffer">A pointer to a AT_JobInfo struct.</param>
        /// <param name="JobId">An output parameter.</param>
        /// <returns>0 on success. An Error code otherwise.</returns>
        [DllImport("Netapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        internal static extern int NetScheduleJobAdd
            (string Servername, IntPtr Buffer, out int JobId);
    }
}
