import subprocess
import csv
import re
import os

# Fill out these dictionaries manually
commands = {
    'spmv': '/u/rgq5aw/GIT/gunrock/build/bin/spmv',
    'sssp': '/u/rgq5aw/GIT/gunrock/build/bin/sssp',
    'pr': '/u/rgq5aw/GIT/gunrock/build/bin/pr',
    'bfs': '/u/rgq5aw/GIT/gunrock/build/bin/bfs',
    'bc': '/u/rgq5aw/GIT/gunrock/build/bin/bc'
}

directories = {
    'SLJ1': '/bigtemp/rgq5aw/graphDatasets/SLJ1',
    'RD': '/bigtemp/rgq5aw/graphDatasets/RD',
    'CPAT': '/bigtemp/rgq5aw/graphDatasets/CPAT',
    'SPKC': '/bigtemp/rgq5aw/graphDatasets/SPKC',
    'CORKT': '/bigtemp/rgq5aw/graphDatasets/CORKT',
    'WIKLE': '/bigtemp/rgq5aw/graphDatasets/WIKLE',
    'GPLUS': '/bigtemp/rgq5aw/graphDatasets/GPLUS',
    'WEB01' : '/bigtemp/rgq5aw/graphDatasets/WEB01',
    'TWTR' : '/bigtemp/rgq5aw/graphDatasets/TWTR',
}

file_names = {
    'graph_0': 'graph_0.mtx',
    'graph_5': 'graph_5.mtx',
    'graph_8': 'graph_8.mtx',
    'graph_9': 'graph_9.mtx',
    'graph_10': 'graph_10.mtx',
    'graph_11': 'graph_11.mtx'
}

# Function to run a command and return its output
def run_command(command):
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    return result.stdout, result.stderr

# Function to parse the elapsed time from the command output
def parse_time(output):
    match = re.search(r'Average GPU Elapsed Time\s*:\s*(\d+\.\d+)\s*\(ms\)', output)
    if match:
        return float(match.group(1))
    return None


# Iterate over directories and file names
for cmd_key, command in commands.items():
    # Prepare CSV data
    csv_data = []
    print("Command [", cmd_key, "] is running ...")
    for dir_key, directory in directories.items():
        row = [dir_key]
        for file_key, file_name in file_names.items():
            full_path = f"{directory}/{file_name}"
            full_command = f"{command} -m {full_path} -n 100"
            print(full_command)
            output = run_command(full_command)
            print("output is: ", output[0])
            print("error is: ", output[1])
            print('-------------------------------------------------')
            elapsed_time = parse_time(output[0])
            row.append(elapsed_time if elapsed_time is not None else 'N/A')
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
