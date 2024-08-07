import subprocess
import csv
import re
import os
import sys


if len(sys.argv) < 2:
    print("Usage: python3 gapExperiments.py <graph_dir>")
    exit(1)
graph_dir = sys.argv[1]
# Ensure graph_dir ends with a '/'
if not graph_dir.endswith('/'):
    graph_dir += '/'
print("graph_dir is: ", graph_dir)

# Fill out these dictionaries manually
commands = {
    'spmv': '/u/rgq5aw/GIT/gunrock/build/bin/spmv',
    'sssp': '/u/rgq5aw/GIT/gunrock/build/bin/sssp',
    'pr': '/u/rgq5aw/GIT/gunrock/build/bin/pr',
    'bfs': '/u/rgq5aw/GIT/gunrock/build/bin/bfs',
    'bc': '/u/rgq5aw/GIT/gunrock/build/bin/bc',
    # 'async_bfs': '/u/rgq5aw/GIT/gunrock/build/bin/async_bfs',
    # 'hits': '/u/rgq5aw/GIT/gunrock/build/bin/hits',
}

directories = {
    'SLJ1': graph_dir + 'SLJ1',
    'RD': graph_dir + 'RD',
    'CPAT': graph_dir + 'CPAT',
    'SPKC': graph_dir + 'SPKC',
    'CORKT': graph_dir + 'CORKT',
    'WIKLE': graph_dir + 'WIKLE',
    'GPLUS': graph_dir + 'GPLUS',
    'WEB01' : graph_dir + 'WEB01',
    'TWTR' : graph_dir + 'TWTR',
}

file_names = {
    'graph_0': 'graph_0.mtx',
    'graph_5': 'graph_5.mtx',
    'graph_8': 'graph_8.mtx',
    'graph_9': 'graph_9.mtx',
    'graph_10': 'graph_10.mtx',
    'graph_11': 'graph_11.mtx',
    # 'graph_12': 'graph_12.mtx',
    # 'graph_13': 'graph_13.mtx'
    'graph_12:0.25': 'graph_12:0.25.mtx',
    'graph_12:0.5': 'graph_12:0.5.mtx',
    'graph_12:0.75': 'graph_12:0.75.mtx',
    'graph_12:1.0': 'graph_12:1.0.mtx',
    'graph_12:1.25': 'graph_12:1.25.mtx',
    'graph_12:1.75': 'graph_12:1.75.mtx',
    'graph_12:2.0': 'graph_12:2.0.mtx',

    'graph_13:15:5:0.25' : 'graph_13:15:5:0.25.mtx',
    'graph_13:15:5:1.0' : 'graph_13:15:5:1.0.mtx',
    'graph_13:15:5:1.75' : 'graph_13:15:5:1.75.mtx',
    'graph_13:15:8:0.25' : 'graph_13:15:8:0.25.mtx',
    ' graph_13:15:8:1.0' : 'graph_13:15:8:1.0.mtx',
    'graph_13:15:8:1.75' : 'graph_13:15:8:1.75.mtx',
    'graph_13:15:9:0.25' : 'graph_13:15:9:0.25.mtx',
    'graph_13:15:9:1.0' : 'graph_13:15:9:1.0.mtx',
    'graph_13:15:9:1.75' : 'graph_13:15:9:1.75.mtx',
    'graph_13:15:10:0.25' : 'graph_13:15:10:0.25.mtx',
    'graph_13:15:10:1.0' : 'graph_13:15:10:1.0.mtx',
    'graph_13:15:10:1.75' : 'graph_13:15:10:1.75.mtx',
    'graph_13:15:11:0.25' : 'graph_13:15:11:0.25.mtx',
    'graph_13:15:11:1.0' : 'graph_13:15:11:1.0.mtx',
    'graph_13:15:11:1.75' : 'graph_13:15:11:1.75.mtx',
    'graph_13:15:12:0.25' : 'graph_13:15:12:0.25.mtx',
    'graph_13:15:12:1.00' : 'graph_13:15:12:1.00.mtx',
    'graph_13:15:12:1.75' : 'graph_13:15:12:1.75.mtx'
}

# Function to run a command and return its output
def run_command(command):
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    return result.stdout, result.stderr

# Function to parse the elapsed time from the command output
def parse_time(output):
    match = re.search(r'\s*GPU Elapsed Time\s*:\s*(\d+\.\d+)\s*\(ms\)', output)
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
            full_command = f"{command} -m {full_path} -n 20"
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
