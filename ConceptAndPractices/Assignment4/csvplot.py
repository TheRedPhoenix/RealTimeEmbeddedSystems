import pandas as pd
import matplotlib.pyplot as plt
import os

csv_extension = ".csv"

script_directory = os.path.dirname(os.path.realpath(__file__))

def get_csv_files_from_path(csv_folder):
    csv_files = []
    for csvfile in os.listdir(csv_folder):
        if csvfile.endswith(csv_extension):
            csv_files.append(csvfile)
    return csv_files

def make_plot(csv_file_list, plot_filename):
    figure, (ax0, ax1) = plt.subplots(2,1)
    figure.set_size_inches(18.5, 10.5)
    ax0.set_title("Clock Time Comparison")
    ax1.set_title("Delay Error Comparison")

    ax0.set_xlabel("iterations")
    ax0.set_ylabel("iteration time [s]")
    ax1.set_xlabel("iterations")
    ax1.set_ylabel("delay error [s]")

    time_handle_entries = []
    delay_handle_entries = []
    legend_entries = []

    for csvfile in csv_file_list:
        df = pd.read_csv(csvfile, delimiter=";")
        file_header = df.columns;
    
        time_handle_entries.append(ax0.plot(df[file_header[0]], '--.', label=csvfile[0:-len(csv_extension)]))
        delay_handle_entries.append(ax1.plot(df[file_header[1]], '--.', label=csvfile[0:-len(csv_extension)]))
        legend_entries.append(csvfile)
    
    ax0.legend(fancybox=True, shadow=True, loc="upper left")
    ax1.legend(fancybox=True, shadow=True, loc="upper left")
    plt.savefig(plot_filename, dpi=300)


if __name__ == '__main__':
    
    csv_files = get_csv_files_from_path(script_directory)

    if len(csv_files) == 0:
        print("Error: no csv input files to produce plot.")
        exit(0)

    # Create a plot with info from all files
    make_plot(csv_files, "comparison_all.png")

    # Create a plot without the *_coarse.csv files information 
    stripped_list = [csv_file for csv_file in csv_files if '_coarse'not in csv_file]
    make_plot(stripped_list, "comparison_without_coarse.png")

