import subprocess
import csv
import re
import os

# Fill out these dictionaries manually
commands = {
    'pr': '/home/rgq5aw/ligra/apps/PageRank',
    'bfs': '/home/rgq5aw/ligra/apps/BFS',
    'bc': '/home/rgq5aw/ligra/apps/BC'
}

directories = {
    'SLJ1': '/media/Data/00_GraphDatasets/GBREW/SLJ1',
    'RD': '/media/Data/00_GraphDatasets/GBREW/RD',
    'CPAT': '/media/Data/00_GraphDatasets/GBREW/CPAT',
    'SPKC': '/media/Data/00_GraphDatasets/GBREW/SPKC',
    'CORKT': '/media/Data/00_GraphDatasets/GBREW/CORKT',
    'WIKLE': '/media/Data/00_GraphDatasets/GBREW/WIKLE',
    'GPLUS': '/media/Data/00_GraphDatasets/GBREW/GPLUS',
    'WEB01' : '/media/Data/00_GraphDatasets/GBREW/WEB01',
    'TWTR' : '/media/Data/00_GraphDatasets/GBREW/TWTR',
}

file_names = {
    'graph_0': 'graph_0.ligra',
    'graph_5': 'graph_5.ligra',
    'graph_8': 'graph_8.ligra',
    'graph_9': 'graph_9.ligra',
    'graph_10': 'graph_10.ligra',
    'graph_11': 'graph_11.ligra'
}

# Function to run a command and return its output
def run_command(command):
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    return result.stdout, result.stderr

# Function to parse the elapsed time from the command output
def extract_running_times(output):
    # Modify the regular expression to capture scientific notation
    running_times = re.findall(r'Running time\s*:\s*([\d\.eE\+\-]+)', output)
    # Convert the captured string(s) to regular numbers
    running_times_regular = []
    for time in running_times:
        try:
            num = float(time)
        except ValueError:
            num = num
        # Check if the original string is in integer format
        if '.' not in time and 'e' not in time.lower():
            running_times_regular.append(int(num))
        else:
            running_times_regular.append(num)

    # Convert the extracted string values to floats
    if running_times_regular:
        return running_times_regular
    # print("No running times found")
    return None

def compute_average(running_times):
    if not running_times:
        return None
    return sum(running_times) / len(running_times)

# Iterate over directories and file names
for cmd_key, command in commands.items():
    # Prepare CSV data
    csv_data = []
    print("Command [", cmd_key, "] is running ...")
    for dir_key, directory in directories.items():
        row = [dir_key]
        for file_key, file_name in file_names.items():
            threads = "OMP_NUM_THREADS=32"
            full_path = f"{directory}/{file_name}"
            full_command = f"{threads} {command} -s {full_path}"
            print(full_command)
            output = run_command(full_command)
            print("output is: ")
            print(output[0])
            print("error is: ")
            print(output[1])
            print('-------------------------------------------------')
            # Extract running times from the output
            running_times = extract_running_times(output[0])
            # Compute the average running time
            average_running_time = compute_average(running_times)
            # print("Average running time: ", average_running_time)
            row.append(average_running_time if average_running_time is not None else 'N/A')
        csv_data.append(row)

    # Write CSV data to a file
    file_name = cmd_key + '.csv'
    csv_headers = ['Graph_Dir'] + list(file_names.values())
    with open(file_name, 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        csvwriter.writerow(csv_headers)
        csvwriter.writerows(csv_data)

    print('CSV file created:', file_name)
    print('-------------------------------------------------')
