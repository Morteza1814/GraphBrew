import os
import re
import subprocess
import csv

max_threads = os.cpu_count()

# Define the base directory containing the graph datasets
BASE_DIR   = "/bigtemp/rgq5aw/graphDatasets"
BASE_NVME_DIR   = "/bigtemp/rgq5aw/graphDatasets_5"
RESULT_DIR = "bench/results"
PARALLEL = os.cpu_count()  # Use all available CPU cores
LOG_DIR_RUN   = os.path.join(RESULT_DIR, "logs_run")
LOG_DIR_ORDER = os.path.join(RESULT_DIR, "logs_order")
os.makedirs(LOG_DIR_RUN, exist_ok=True)
os.makedirs(LOG_DIR_ORDER, exist_ok=True)

# Define the list of graphs and their extensions
graph_extensions = {
    "SLJ1": "sg",
    "RD": "sg",
    "CPAT": "sg",
    "CORKT": "sg",
    "SPKC": "sg",
    "GPLUS": "sg",
    "WIKLE": "sg",
    "WEB01": "sg",
    "TWTR": "sg"
}

# Define the list of kernels
kernels = [
    {"name": "bc", "trials": 20, "iterations": 10},
    {"name": "bfs", "trials": 20, "iterations": 10},
    {"name": "cc", "trials": 20, "iterations": 10},
    {"name": "cc_sv", "trials": 20, "iterations": 10},
    {"name": "pr", "trials": 10, "iterations": 200},
    {"name": "pr_spmv", "trials": 10, "iterations": 200},
    {"name": "sssp", "trials": 20, "iterations": 10}
]

# Regular expressions for parsing timing data from benchmark outputs
time_patterns = {
    "reorder_time": {
        "GraphBrew": re.compile(r"\bGraphBrewOrder\b Map Time:\s*([\d\.]+)"),
        "HubClusterDBG": re.compile(r"\bHubClusterDBG\b Map Time:\s*([\d\.]+)"),
        "HubCluster": re.compile(r"\bHubCluster\b Map Time:\s*([\d\.]+)"),
        "HubSortDBG": re.compile(r"\bHubSortDBG\b Map Time:\s*([\d\.]+)"),
        "HubSort": re.compile(r"\bHubSort\b Map Time:\s*([\d\.]+)"),
        "Leiden": re.compile(r"\bLeidenOrder\b Map Time:\s*([\d\.]+)"),
        "Original": re.compile(r"\bOriginal\b Map Time:\s*([\d\.]+)"),
        "RabbitOrder": re.compile(r"\bRabbitOrder\b Map Time:\s*([\d\.]+)"),
        "Random": re.compile(r"\bRandom\b Map Time:\s*([\d\.]+)"),
        "Corder": re.compile(r"\bCOrder\b Map Time:\s*([\d\.]+)"),
        "Gorder": re.compile(r"\bGOrder\b Map Time:\s*([\d\.]+)"),
        "DBG": re.compile(r"\bDBG\b Map Time:\s*([\d\.]+)"),
        "RCM": re.compile(r"\bRCMOrder\b Map Time:\s*([\d\.]+)"),
        "Sort": re.compile(r"\bSort\b Map Time:\s*([\d\.]+)")
    },
    "trial_time": {
        "Average": re.compile(r"\bAverage\b Time:\s*([\d\.]+)")
    }
}

reorder_option_mapping = {
    "GraphBrew1": "-o13:10:1",
    "GraphBrew5": "-o13:10:5",
    "GraphBrew8": "-o13:10:8",
    "GraphBrew9": "-o13:10:9",
    "GraphBrew10": "-o13:10:10",
    "GraphBrew11": "-o13:10:11",
    "GraphBrew12": "-o13:10:12"
}

single_reorder_option_mapping = {
    # "Random": "-o0", # this is your baseline
    # "Sort": "-o2",
    # "HubSort": "-o3",
    # "HubCluster": "-o4",
    # "DBG": "-o5",
    # "HubSortDBG": "-o6",
    # "HubClusterDBG": "-o7",
    "RabbitOrder": "-o8",
    # "Gorder": "-o9",
    # "Corder": "-o10",
    # "RCM": "-o11",
    "GraphBrew_12_025": "-o12:0.25",
    "GraphBrew_12_050": "-o12:0.5",
    "GraphBrew_12_075": "-o12:0.75",
    "GraphBrew_12_100": "-o12:1.0",
    "GraphBrew_12_125": "-o12:1.25",
    "GraphBrew_12_175": "-o12:1.75",
    "GraphBrew_12_200": "-o12:2.0",
    "GraphBrew_13_15_5_025"  : "-o13:15:5:0.25",
    "GraphBrew_13_15_5_100"  : "-o13:15:5:1.0",
    "GraphBrew_13_15_5_175"  : "-o13:15:5:1.75",
    "GraphBrew_13_15_8_025"  : "-o13:15:8:0.25",
    "GraphBrew_13_15_8_100"  : "-o13:15:8:1.0",
    "GraphBrew_13_15_8_175"  : "-o13:15:8:1.75",
    "GraphBrew_13_15_9_025"  : "-o13:15:9:0.25",
    "GraphBrew_13_15_9_100"  : "-o13:15:9:1.0",
    "GraphBrew_13_15_9_175"  : "-o13:15:9:1.75",
    "GraphBrew_13_15_10_025" : "-o13:15:10:0.25",
    "GraphBrew_13_15_10_100" : "-o13:15:10:1.0",
    "GraphBrew_13_15_10_175" : "-o13:15:10:1.75",
    "GraphBrew_13_15_11_025" : "-o13:15:11:0.25",
    "GraphBrew_13_15_11_100" : "-o13:15:11:1.0",
    "GraphBrew_13_15_11_175" : "-o13:15:11:1.75",
    "GraphBrew_13_15_12_025" : "-o13:15:12:0.25",
    "GraphBrew_13_15_12_100" : "-o13:15:12:1.00",
    "GraphBrew_13_15_12_175" : "-o13:15:12:1.75",
}

# reorder_option_mapping = {
#     # "Random": "-o0", # this is your baseline
#     "DBG": "-o5",
#     "RabbitOrder": "-o8 -o5",
#     "Gorder": "-o9 -o5",
#     "Corder": "-o10 -o5",
#     "RCM": "-o11 -o5",
#     "Leiden": "-o12 -o5"
# }

def parse_reorder_output(output):
    timings = {}
    for key, pattern in time_patterns["reorder_time"].items():
        match = pattern.search(output)
        if match:
            timings[key] = float(match.group(1))
    return timings

def parse_kernel_output(output):
    match = time_patterns["trial_time"]["Average"].search(output)
    if match:
        return float(match.group(1))
    return None

def run_reorders():
    print("Starting reorder process...")
    
    results = {}

    affinity = "0-31"  # Specify CPU IDs from 0 to 31
    os.environ["GOMP_CPU_AFFINITY"] = affinity
    print(f"Setting GOMP_CPU_AFFINITY to {affinity}")
    
    # Iterate over each graph
    for graph, ext in graph_extensions.items():
        print(f"Processing graph: {graph}")
        
        # Construct the graph file path
        graph_file = os.path.join(BASE_DIR, graph, f"graph.{ext}")
        random_graph_file = os.path.join(BASE_DIR, graph, f"graph_0.sg")

        reorder_name   = "Random" 
        reorder_option = "-o1"        
        # Construct a random graph if it does not exist
        if not os.path.isfile(random_graph_file):
            print(f"Running converter with reorder {reorder_name} option: {reorder_option}")
            print(f"Output file: {random_graph_file}")
<<<<<<< HEAD
            make_command = f"make run-converter GRAPH_BENCH='-f {graph_file} -b {random_graph_file}' RUN_PARAMS='{reorder_option}' FLUSH_CACHE=0 PARALLEL={max_threads}"
=======
            make_command = f"make run-converter GRAPH_BENCH='-f {graph_file} -b {random_graph_file}' RUN_PARAMS='{reorder_option}' FLUSH_CACHE=0 PARALLEL={PARALLEL}"
>>>>>>> 5a6b641f0cf668977c52d5e825e1d8e55d8243ab
            log_file = os.path.join(LOG_DIR_ORDER, f"{graph}_initial.log")
            with open(log_file, 'w') as log:
                print(f"Executing command: {make_command}")
                subprocess.run(make_command, shell=True, check=True, stdout=log, stderr=log)
        
        # Check if the random graph file exists
        if os.path.isfile(random_graph_file):

            print(f"Graph file found: {random_graph_file}")
            
            results[graph] = {}
            
            # Iterate over each reorder option
            for reorder_name, reorder_option in list(single_reorder_option_mapping.items()):
                if ' ' in reorder_option:
                    # Handle multiple options
                    option_numbers = '_'.join([opt.split('o')[1] for opt in reorder_option.split()])
                    output_file = os.path.join(BASE_NVME_DIR, graph, f"graph_{option_numbers}.sg")
                else:
                    # Handle single option
                    option_number = reorder_option.split('o')[1]
                    output_file = os.path.join(BASE_NVME_DIR, graph, f"graph_{option_number}.sg")

                
                # Ensure the graph directories exist
                os.makedirs(os.path.join(BASE_DIR, graph), exist_ok=True)
                os.makedirs(os.path.join(BASE_NVME_DIR, graph), exist_ok=True)

                # Skip if the output file already exists
                if os.path.isfile(output_file):
                    print(f"Output file already exists, skipping: {output_file}")
                    continue
                
                # Print the current stage
                print(f"Running converter with reorder {reorder_name} option: {reorder_option}")
                print(f"Output file: {output_file}")
                
                # Construct and run the make command
                make_command = f"make run-converter GRAPH_BENCH='-f {random_graph_file} -b {output_file}' RUN_PARAMS='{reorder_option}' FLUSH_CACHE=0 PARALLEL={PARALLEL}"
                log_file = os.path.join(LOG_DIR_ORDER, f"{graph}_{reorder_name}.log")
                with open(log_file, 'w') as log:
                    print(f"Executing command: {make_command}")
                    result = subprocess.run(make_command, shell=True, check=True, stdout=log, stderr=log)
                
                # Parse the output from the log file
                with open(log_file, 'r') as log:
                    timings = parse_reorder_output(log.read())
                
                # Record the results
                for key, time in timings.items():
                    if reorder_name in single_reorder_option_mapping:
                        results[graph][reorder_name] = time
                
                print(f"Completed conversion for reorder option: {reorder_option}\n")
        else:
            print(f"Graph file not found: {random_graph_file}")
    
    # Check if results are empty
    if not results:
        print("No new conversions were performed. All graph files already exist.")
        return
    
    # Write results to CSV
    csv_file = os.path.join(RESULT_DIR, "reorder_results.csv")
    with open(csv_file, mode='w', newline='') as file:
        writer = csv.writer(file)
        header = ["Graph"] + list(single_reorder_option_mapping.keys())
        writer.writerow(header)
        
        for graph, timings in results.items():
            row = [graph] + [timings.get(reorder_name, '') for reorder_name in single_reorder_option_mapping.keys()]
            writer.writerow(row)
    
    print("Reorder process completed.")

def run_kernels():
    print("Starting kernel execution process...")
    
    kernel_results = {kernel["name"]: {} for kernel in kernels}
    
    # Iterate over each graph
    for graph in graph_extensions.keys():
        print(f"Processing graph: {graph}")
        
        # Iterate over each reorder option
        for reorder_name, reorder_option in single_reorder_option_mapping.items():
            if ' ' in reorder_option:
                # Handle multiple options
                option_numbers = '_'.join([opt.split('o')[1] for opt in reorder_option.split()])
                output_file = os.path.join(BASE_DIR, graph, f"graph_{option_numbers}.sg")
            else:
                # Handle single option
                option_number = reorder_option.split('o')[1]
                output_file = os.path.join(BASE_DIR, graph, f"graph_{option_number}.sg")
            
            # Check if the converted graph file exists
            if os.path.isfile(output_file):
                print(f"Converted graph file found: {output_file}")
                
                # Run kernels on the converted graph file
                for kernel in kernels:
                    kernel_command = f"make run-{kernel['name']} GRAPH_BENCH='-f {output_file}' RUN_PARAMS='-l -n {kernel['trials']}' FLUSH_CACHE=1 PARALLEL={PARALLEL}"
                    if kernel["name"] in ["pr", "pr_spmv"]:
                        kernel_command = f"make run-{kernel['name']} GRAPH_BENCH='-f {output_file}' RUN_PARAMS='-l -n {kernel['trials']} -i {kernel['iterations']}' FLUSH_CACHE=1 PARALLEL={PARALLEL}"
                    log_file = os.path.join(LOG_DIR_RUN, f"{graph}_{reorder_name}_{kernel['name']}.log")
                    print(f"Running kernel: {kernel['name']} with {kernel['trials']} trials and {kernel['iterations']} iterations")
                    print(f"Executing command: {kernel_command}")
                    
                    # # Run the command and log the output
                    # with open(log_file, 'w') as log:
                    #     result = subprocess.run(kernel_command, shell=True, check=True, stdout=log, stderr=log)
                    
                    # Parse the output from the log file
                    with open(log_file, 'r') as log:
                        average_time = parse_kernel_output(log.read())
                    
                    if average_time is not None:
                        if graph not in kernel_results[kernel['name']]:
                            kernel_results[kernel['name']][graph] = {}
                        kernel_results[kernel['name']][graph][reorder_name] = average_time
                    
                    print(f"Completed kernel: {kernel['name']}\n")
            else:
                print(f"Converted graph file not found: {output_file}")
    
    # Check if kernel results are empty
    if all(not results for results in kernel_results.values()):
        print("No kernels were executed. All converted graph files already exist or were not found.")
        return
    
    # Write results to CSV for each kernel
    for kernel_name, results in kernel_results.items():
        if results:
            csv_file = os.path.join(RESULT_DIR, f"{kernel_name}_trial_time_results.csv")
            with open(csv_file, mode='w', newline='') as file:
                writer = csv.writer(file)
                header = ["Graph"] + list(single_reorder_option_mapping.keys())
                writer.writerow(header)
                
                for graph, timings in results.items():
                    row = [graph] + [timings.get(reorder_name, '') for reorder_name in single_reorder_option_mapping.keys()]
                    writer.writerow(row)
    
    print("Kernel execution process completed.")

def run_kernels_affin():
    print("Starting kernel execution process...")

    # Define different CPU affinity settings to experiment with
    affinities = [
        "0-15",  # First 16 physical cores
        "0-15:2",  # Every second core in the first 16 cores
        "16-31",  # Last 16 logical cores (Hyper-threaded pairs of the first 16 cores)
        "0-31",  # All 32 threads
        "0-31:2"  # Every second thread in all 32 threads
    ]

    kernel_results = {kernel["name"]: {} for kernel in kernels}

    # Iterate over each graph
    for graph in graph_extensions.keys():
        print(f"Processing graph: {graph}")

        # Iterate over each reorder option
        for reorder_name, reorder_option in single_reorder_option_mapping.items():
            if ' ' in reorder_option:
                # Handle multiple options
                option_numbers = '_'.join([opt.split('o')[1] for opt in reorder_option.split()])
                output_file = os.path.join(BASE_DIR, graph, f"graph_{option_numbers}.sg")
            else:
                # Handle single option
                option_number = reorder_option.split('o')[1]
                output_file = os.path.join(BASE_DIR, graph, f"graph_{option_number}.sg")

            # Check if the converted graph file exists
            if os.path.isfile(output_file):
                print(f"Converted graph file found: {output_file}")

                # Run kernels on the converted graph file with different affinity settings
                for affinity in affinities:
                    os.environ["GOMP_CPU_AFFINITY"] = affinity
                    print(f"Setting GOMP_CPU_AFFINITY to {affinity}")

                    for kernel in kernels:
                        kernel_command = f"make run-{kernel['name']} GRAPH_BENCH='-f {output_file}' RUN_PARAMS='-l -n {kernel['trials']}' FLUSH_CACHE=1 PARALLEL={PARALLEL}"
                        if kernel["name"] in ["pr", "pr_spmv"]:
                            kernel_command = f"make run-{kernel['name']} GRAPH_BENCH='-f {output_file}' RUN_PARAMS='-l -n {kernel['trials']} -i {kernel['iterations']}' FLUSH_CACHE=1 PARALLEL={PARALLEL}"
                        log_file = os.path.join(LOG_DIR_RUN, f"{graph}_{reorder_name}_{kernel['name']}_{affinity.replace(' ', '_')}.log")
                        print(f"Running kernel: {kernel['name']} with {kernel['trials']} trials and {kernel['iterations']} iterations")
                        print(f"Executing command: {kernel_command}")

                        # Run the command and log the output
                        with open(log_file, 'w') as log:
                            result = subprocess.run(kernel_command, shell=True, check=True, stdout=log, stderr=log)

                        # Parse the output from the log file
                        with open(log_file, 'r') as log:
                            average_time = parse_kernel_output(log.read())

                        if average_time is not None:
                            if graph not in kernel_results[kernel['name']]:
                                kernel_results[kernel['name']][graph] = {}
                            kernel_results[kernel['name']][graph][reorder_name] = average_time

                        print(f"Completed kernel: {kernel['name']} with affinity {affinity}\n")
            else:
                print(f"Converted graph file not found: {output_file}")

    # Check if kernel results are empty
    if all(not results for results in kernel_results.values()):
        print("No kernels were executed. All converted graph files already exist or were not found.")
        return

    # Write results to CSV for each kernel
    for kernel_name, results in kernel_results.items():
        if results:
            csv_file = os.path.join(RESULT_DIR, f"{kernel_name}_trial_time_results.csv")
            with open(csv_file, mode='w', newline='') as file:
                writer = csv.writer(file)
                header = ["Graph"] + list(single_reorder_option_mapping.keys())
                writer.writerow(header)

                for graph, timings in results.items():
                    row = [graph] + [timings.get(reorder_name, '') for reorder_name in single_reorder_option_mapping.keys()]
                    writer.writerow(row)

    print("Kernel execution process completed.")

def run_convert():
    print("Starting reorder process...")
    
    results = {}
    
    # Iterate over each graph
    for graph, ext in graph_extensions.items():
        print(f"Processing graph: {graph}")
        
        results[graph] = {}
            
        # Iterate over each reorder option
        for reorder_name, reorder_option in list(single_reorder_option_mapping.items()):
            if ' ' in reorder_option:
                # Handle multiple options
                option_numbers = '_'.join([opt.split('o')[1] for opt in reorder_option.split()])
                output_file = os.path.join(BASE_DIR, graph, f"graph_{option_numbers}.sg")
                output_file_conv = os.path.join(BASE_NVME_DIR, graph, f"graph_{option_numbers}.sg")
            else:
                # Handle single option
                option_number = reorder_option.split('o')[1]
                output_file = os.path.join(BASE_DIR, graph, f"graph_{option_number}.sg")
                output_file_conv = os.path.join(BASE_NVME_DIR, graph, f"graph_{option_number}.sg")
            
            # Ensure the graph directories exist
            os.makedirs(os.path.join(BASE_DIR, graph), exist_ok=True)
            os.makedirs(os.path.join(BASE_NVME_DIR, graph), exist_ok=True)

            # Skip if the output file already exists
            if os.path.isfile(output_file_conv):
                print(f"Output file already exists, skipping: {output_file_conv}")
                continue
            
            # Print the current stage
            print(f"Running converter with reorder {reorder_name} option: {reorder_option}")
            print(f"Output file: {output_file}")
            
            # Construct and run the make command
<<<<<<< HEAD
            make_command = f"make run-converter GRAPH_BENCH='-f {output_file} -b {output_file_conv} -p {output_file_conv}' RUN_PARAMS='-o5' FLUSH_CACHE=0 PARALLEL={max_threads}"
=======
            make_command = f"make run-converter GRAPH_BENCH='-f {output_file} -b {output_file_conv} -p {output_file_conv}' RUN_PARAMS='-o5' FLUSH_CACHE=0 PARALLEL={PARALLEL}"
>>>>>>> 5a6b641f0cf668977c52d5e825e1d8e55d8243ab
            log_file = os.path.join(LOG_DIR_ORDER, f"{graph}_{reorder_name}.log")
            with open(log_file, 'w') as log:
                print(f"Executing command: {make_command}")
                result = subprocess.run(make_command, shell=True, check=True, stdout=log, stderr=log)
            
            # Parse the output from the log file
            with open(log_file, 'r') as log:
                timings = parse_reorder_output(log.read())
            
            # Record the results
            for key, time in timings.items():
                if reorder_name in single_reorder_option_mapping:
                    results[graph][reorder_name] = time
            
            print(f"Completed conversion for reorder option: {reorder_option}\n")
    
    print("Convert process completed.")

if __name__ == "__main__":
    # run_convert()
    run_reorders()
    # run_kernels()
    # run_kernels_affin()
