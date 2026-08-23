import subprocess 

results = []
for i in range(30):
    process = subprocess.run(['../../build/throughput', 's'], capture_output=True, text=True)
    ops_per_sec = float(process.stdout.strip())
    results.append(ops_per_sec)

print(f"Small allocations throughput: {(results[15] + results[16]) / 2}")

results = [] 

for i in range(30):
    process = subprocess.run(['../../build/throughput', 'l'], capture_output=True, text=True)
    ops_per_sec = float(process.stdout.strip())
    results.append(ops_per_sec)


print(f"Large allocations throughput: {(results[15] + results[16]) / 2}")




