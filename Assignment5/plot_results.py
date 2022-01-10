import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import re
from enum import Enum

plt.style.use('classic')

# This python script reads the syslog and isolates the lines related to seqgen3
# thanks to inclueded patterns ==== START LOGGING ==== and ==== STOP LOGGING ====,
# transformes the retrieved information in a dataframe and plots the behavior
# of the release time of the various frequencies

# Seconds to milliseconds conversion factor
seconds_to_milliseconds = 1000

# Path to the syslog file
syslog_filepath = "/var/log/syslog"

# name of columns in the dataframe (and of groups in the regex)
frequency_column_name="frequency"
core_used_column_name = "core"
release_number_column_name = "release"
release_time_column_name = "releaseTime"

# Regular Expression matching the lines printed out by the 
# seqgen3 program. created with the support of regex101.com
line_match_expression =(r"^.*seqgen3:\sS\d\s(?P<{freq}>[0-9]+([.][0-9]+)?)\sHz"
                        r"\son\score\s(?P<{core_used}>[0-9]+)\sfor\srelease\s(?P<{release_number}>[0-9]+)"
                        r"\s@\ssec=(?P<{release_time}>[0-9]+([.][0-9]+)?)").format(
                            freq = frequency_column_name,
                            core_used=core_used_column_name, 
                            release_number = release_number_column_name,
                            release_time=release_time_column_name)

start_session_pattern   = r"^.*seqgen3:\s==== START LOGGING ===="
stop_session_pattern    = r"^.*seqgen3:\s==== STOP LOGGING ===="

class ParsingStatus(Enum):
    NotStarted = 1,
    InProgress = 2,
    Finished   = 3

def read_syslog(path_to_syslog):

    # Set the initial parsing status to NotStarted
    reading_status = ParsingStatus.NotStarted

    # Create an empty list to host the findings
    records = []

    # Open the input file path, and process it in reverse order
    for line in reversed(list(open(path_to_syslog))):

        # Handle cases:
        # 1 Parsing not started yet --> analyse if Stop Session pattern can be matched
        # 2 Parsing in progress --> check if current line matches line pattern
        # 3 Parsing in progress --> analyse if Start Session pattern can be matched. If so, exit

        # Case 1: Session not started yet, so check for "Stop Session" pattern
        if(reading_status == ParsingStatus.NotStarted and re.match(stop_session_pattern, line)):
            # Found stop session pattern. It means that, reading in reverse order
            # we can start parsing the file
            reading_status = ParsingStatus.InProgress

        # Case 2: Session is in progress, check if the current line contains information  
        if(reading_status == ParsingStatus.InProgress and re.match(line_match_expression, line)):
            # If so, parse and append the info to the records array
            records.append(re.search(line_match_expression, line).groupdict())
        
        # Case 3:Session in progress, so check if "start session" pattern can be matched
        if(reading_status == ParsingStatus.InProgress and re.match(start_session_pattern, line)):
            # Found start session pattern. It means that, reading in reverse order
            # the end of the session has been reached
            reading_status = ParsingStatus.Finished
            # Can break out of the for loop        
            break

    # Parsing completed! Return the reversed list to match
    # the original order in the file
    return reversed(records)

if __name__ == "__main__":

    # Extract records from Syslog file
    extracted_records = read_syslog(syslog_filepath)

    # Converte extracted records to dataframe
    dataframe = pd.DataFrame(extracted_records)

    # Force dataframe column type
    dataframe = dataframe.astype({frequency_column_name : np.float32,
                                     core_used_column_name : int,
                                      release_number_column_name : int,
                                      release_time_column_name : np.float32})
    
    # Grouping dataframe by frequencies
    grouped = dataframe.groupby(frequency_column_name)

    # Create a list of dataframe per specific frequency
    list_of_dataframes = [group for _, group in grouped]

    # Iterate the list of frequencies and plot
    for index, df in enumerate(list_of_dataframes):
        # Obtain current frequencey
        frequency = df.iloc[0][frequency_column_name]
        frequency = np.around(frequency,decimals=2)
        # Reverse frequency to obtain period
        period = 1/frequency
        period = np.round(period, decimals=3)
        # Create figure
        fig = plt.figure(index)
        fig.suptitle("Difference between thread release time at {}Hz".format(frequency))
        # Get the difference between two consecutive release times
        array_of_time_delta_between_releases = np.diff(df[release_time_column_name])
        # Plot the differences between samples
        plt.plot(seconds_to_milliseconds*np.diff(df[release_time_column_name]), '--ob', label='Release Time (ms)')
        # Plot reference period
        plt.plot(seconds_to_milliseconds*period*np.ones(len(df[release_time_column_name])), '-r', label='Reference T={}ms'.format(period*seconds_to_milliseconds))
        # calculate max deviation
        max_deviation = np.abs( period - array_of_time_delta_between_releases ).max()
        plt.figtext(0.5, 0.01, "Max deviation from collection time = {}ms".format(max_deviation*seconds_to_milliseconds), wrap=True, horizontalalignment='center')
        # Set the scientific notation for y axis
        plt.ticklabel_format(axis="both", style="plain", useOffset=False)
        # Set label to axis
        plt.ylabel("Time [ms]")
        # Create plot legend
        plt.legend()
        plt.savefig("./seqgen_{}hz.png".format(frequency), format='png')

    plt.show()
        

