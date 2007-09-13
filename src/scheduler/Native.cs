using System;
using System.Runtime.InteropServices;

// Special thanks to Willy Denoyette for posting the pInvoke code on
// http://www.dotnet247.com/247reference/msgs/47/239675.aspx


namespace TaskScheduler
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
