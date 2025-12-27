import os

def write(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w') as f: f.write(content)

apps = {'kaspan': 'bench_kaspan', 'ispan': 'bench_ispan', 'hpc_graph': 'bench_hpc_graph'}
configs = {
    'single': {'nodes': 1, 'ntasks': 76, 'cpus': 1, 'socket': 38, 'node_tasks': 76, 'is_hpc': {'ntasks': 1, 'socket': 1, 'node_tasks': 1, 'cpus': 76}},
    'medium': {'nodes': 7, 'ntasks': 532, 'cpus': 1, 'socket': 38, 'node_tasks': 76, 'is_hpc': {'ntasks': 7, 'socket': 1, 'node_tasks': 1, 'cpus': 76}},
    '1064': {'nodes': 14, 'ntasks': 1064, 'cpus': 1, 'socket': 38, 'node_tasks': 76, 'is_hpc': {'ntasks': 14, 'socket': 1, 'node_tasks': 1, 'cpus': 76}},
    '2052': {'nodes': 27, 'ntasks': 2052, 'cpus': 1, 'socket': 38, 'node_tasks': 76, 'is_hpc': {'ntasks': 27, 'socket': 1, 'node_tasks': 1, 'cpus': 76}},
    '4104': {'nodes': 54, 'ntasks': 4104, 'cpus': 1, 'socket': 38, 'node_tasks': 76, 'is_hpc': {'ntasks': 54, 'socket': 1, 'node_tasks': 1, 'cpus': 76}},
}
generics = {
    'gnm-directed': 'gnm-directed;n=${n};m=${m};seed=13',
    'rmat_a59_bc19': 'rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13',
    'rmat_abc25': 'rmat;directed;n=${n};m=${m};a=0.25;b=0.25;c=0.25;seed=13'
}

for scaling in ['weak', 'strong']:
    for g_name, kagen in generics.items():
        for comp, app_bin in apps.items():
            for s_name, cfg in configs.items():
                is_hpc = comp == 'hpc_graph'
                c = cfg['is_hpc'] if is_hpc else cfg
                sbatch_name = 'kaspan' if comp == 'ispan' and scaling == 'weak' else comp
                out_err_name = f"{sbatch_name}_{s_name}"
                if comp == 'ispan' and scaling == 'strong' and s_name == 'single': out_err_name = "isapn_single"
                
                header = f"""#!/bin/bash
#SBATCH --nodes={cfg['nodes']}
#SBATCH --ntasks={c['ntasks']}
#SBATCH --cpus-per-task={c['cpus']}
#SBATCH --ntasks-per-socket={c['socket']}
#SBATCH --ntasks-per-node={c['node_tasks']}
#SBATCH -o {out_err_name}.out
#SBATCH -e {out_err_name}.err
#SBATCH -J {'ispan_single' if comp == 'ispan' and scaling == 'strong' and s_name == 'single' else out_err_name}
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_generic.sh"""
                if s_name == 'medium': header += "\nsource ~/workspace/KaSpan/experiment/horeka/run_parallel.sh"
                header += f"\napp_name={comp}\napp=~/workspace/KaSpan/cmake-build-release/bin/{app_bin}\nset +eu"
                
                body = ""
                if scaling == 'weak':
                    if s_name == 'single':
                        body = "\nfor np in 76 38 16 8 4 2 1; do\nfor local_n in 150000 300000 600000; do\nfor d in 90 100 200 400; do\nn=$(( np * local_n ))\nm=$(( n * d / 100 ))\nkagen_string=\"" + kagen + "\"\noutput_file=\"${app_name}_" + g_name + "_np${np}_n${local_n}_d${d}.json\"\n"
                        run_args = "--nodes=1 --ntasks=$np --cpus-per-task=1 --hint=nomultithread" if not is_hpc else "--nodes=1 --ntasks=1 --ntasks-per-socket=1 --cpus-per-task=$np"
                        body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$local_n" "$d" "{g_name}" "{run_args}"{" --threads $np" if is_hpc else ""}\ndone\ndone\ndone\n'
                    elif s_name == 'medium':
                        body = "\nfor local_n in 150000 300000 600000; do\nfor d in 90 100 200 400; do\n"
                        for np_val, nodes_val in [(152, 2), (304, 4)]:
                            body += f"n=$(( {np_val} * local_n ))\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\n"
                            run_args = f'--nodes={nodes_val} --ntasks={np_val if not is_hpc else nodes_val} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                            body += f'run_async run_generic "$app" "${{app_name}}_{g_name}_np{np_val}_n${{local_n}}_d${{d}}.json" "$kagen_string" "$app_name" {np_val} "$local_n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\n'
                        body += "wait_all\ndone\ndone\n# np=532\nfor local_n in 150000 300000 600000; do\nfor d in 90 100 200 400; do\nn=$(( 532 * local_n ))\nm=$(( n * d / 100 ))\nkagen_string=\"" + kagen + "\"\n"
                        run_args = f'--nodes=7 --ntasks={532 if not is_hpc else 7} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                        body += f'run_generic "$app" "${{app_name}}_{g_name}_np532_n${{local_n}}_d${{d}}.json" "$kagen_string" "$app_name" 532 "$local_n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
                    else:
                        body = f"\nnp={cfg['ntasks']}\nfor local_n in 150000 300000 600000; do\nfor d in 90 100 200 400; do\nn=$(( np * local_n ))\n"
                        m_line, kg = "m=$(( n * d / 100 ))", kagen
                        if is_hpc and s_name == '1064' and g_name == 'gnm-directed': m_line, kg = "m=$(( local_n * d / 100 ))", "gnm-directed;n=${local_n};m=${local_m};seed=13"
                        body += f"{m_line}\nkagen_string=\"{kg}\"\noutput_file=\"${{app_name}}_{g_name}_np${{np}}_n${{local_n}}_d${{d}}.json\"\n"
                        run_args = f'--nodes={cfg["nodes"]} --ntasks={cfg["ntasks"] if not is_hpc else cfg["nodes"]} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                        body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$local_n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
                else: # strong
                    if s_name == 'single':
                        body = "\nfor np in 76 38 16 8 4 2 1; do\nfor n in 1000000000 100000000 10000000; do\nfor d in 90 100 200 400; do\nm=$(( n * d / 100 ))\nkagen_string=\"" + kagen + "\"\noutput_file=\"${app_name}_" + g_name + "_np${np}_n${n}_d${d}.json\"\n"
                        run_args = "--nodes=1 --ntasks=$np --cpus-per-task=1 --hint=nomultithread" if not is_hpc else "--nodes=1 --ntasks=1 --ntasks-per-socket=1 --cpus-per-task=$np"
                        body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$n" "$d" "{g_name}" "{run_args}"{" --threads $np" if is_hpc else ""}\ndone\ndone\ndone\n'
                    elif s_name == 'medium':
                        body = "\nfor n in 1000000000 100000000 10000000; do\nfor d in 90 100 200 400; do\nm=$(( n * d / 100 ))\nkagen_string=\"" + kagen + "\"\n"
                        for np_val, nodes_val in [(152, 2), (304, 4)]:
                            run_args = f'--nodes={nodes_val} --ntasks={np_val if not is_hpc else nodes_val} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                            body += f'run_async run_generic "$app" "${{app_name}}_{g_name}_np{np_val}_n${{n}}_d${{d}}.json" "$kagen_string" "$app_name" {np_val} "$n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\n'
                        body += "wait_all\ndone\ndone\n# np=532\nfor n in 1000000000 100000000 10000000; do\nfor d in 90 100 200 400; do\nm=$(( n * d / 100 ))\nkagen_string=\"" + kagen + "\"\n"
                        run_args = f'--nodes=7 --ntasks={532 if not is_hpc else 7} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                        body += f'run_generic "$app" "${{app_name}}_{g_name}_np532_n${{n}}_d${{d}}.json" "$kagen_string" "$app_name" 532 "$n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
                    else: # 1064, 2052, 4104
                        body = f"\nnp={cfg['ntasks']}\nfor n in 1000000000 100000000 10000000; do\nfor d in 90 100 200 400; do\nm=$(( n * d / 100 ))\nkagen_string=\"" + kagen + "\"\noutput_file=\"${app_name}_" + g_name + "_np${np}_n${n}_d${d}.json\"\n"
                        n_run = cfg['ntasks'] if not is_hpc else cfg['nodes']
                        run_args = f'--nodes={cfg["nodes"]} --ntasks={n_run} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                        body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
                write(f"experiment/horeka/{scaling}/generic/{g_name}/{comp}_{s_name}.sh", header + body)

for comp, app_bin in apps.items():
    for s_name, cfg in configs.items():
        if s_name not in ['single', 'medium', '1064']: continue
        is_hpc = comp == 'hpc_graph'
        c = cfg['is_hpc'] if is_hpc else cfg
        header = f"""#!/bin/bash
#SBATCH --nodes={cfg['nodes']}
#SBATCH --ntasks={c['ntasks']}
#SBATCH --cpus-per-task={c['cpus']}
#SBATCH --ntasks-per-socket={c['socket']}
#SBATCH --ntasks-per-node={c['node_tasks']}
#SBATCH -o {comp}_{s_name}.out
#SBATCH -e {comp}_{s_name}.err
#SBATCH -J {comp}_{s_name}
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source ~/workspace/KaSpan/experiment/horeka/env.sh
source ~/workspace/KaSpan/experiment/horeka/run_rwd.sh"""
        if s_name == 'medium': header += "\nsource ~/workspace/KaSpan/experiment/horeka/run_parallel.sh"
        header += f"\napp_name={comp}\napp=~/workspace/KaSpan/cmake-build-release/bin/{app_bin}\nrwd=( ~/workspace/KaSpan/experiment/rwd/*.manifest )\nset +eu\n"
        body = 'for manifest in "${rwd[@]}"; do\n'
        if s_name == 'single':
            body += "for np in 76 38 16 8 4 2 1; do\nmanifest_name=\"$(basename \"${manifest%.manifest}\")\"\noutput_file=\"${app_name}_${manifest_name}_np${np}.json\"\n"
            run_args = f'--nodes=1 --ntasks=$np --cpus-per-task=1 --hint=nomultithread' if not is_hpc else f'--nodes=1 --ntasks=1 --ntasks-per-socket=1 --cpus-per-task=$np'
            body += f'run_rwd "$app" "$output_file" "$manifest" "$app_name" "$np" "$manifest_name" "{run_args}"{" --threads $np" if is_hpc else ""}\ndone\ndone\n'
        elif s_name == 'medium':
            body += "manifest_name=\"$(basename \"${manifest%.manifest}\")\"\n"
            for np_val, nodes_val in [(152, 2), (304, 4)]:
                run_args = f'--nodes={nodes_val} --ntasks={np_val if not is_hpc else nodes_val} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                body += f'run_async run_rwd "$app" "${{app_name}}_${{manifest_name}}_np{np_val}.json" "$manifest" "$app_name" {np_val} "$manifest_name" "{run_args}"{" --threads 76" if is_hpc else ""}\n'
            body += "wait_all\ndone\nfor manifest in \"${rwd[@]}\"; do\nmanifest_name=\"$(basename \"${manifest%.manifest}\")\"\n"
            run_args = f'--nodes=7 --ntasks={532 if not is_hpc else 7} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
            body += f'run_rwd "$app" "${{app_name}}_${{manifest_name}}_np532.json" "$manifest" "$app_name" 532 "$manifest_name" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\n'
        else: # 1064
            body += "manifest_name=\"$(basename \"${manifest%.manifest}\")\"\noutput_file=\"${app_name}_${manifest_name}_np1064.json\"\n"
            run_args = f'--nodes=14 --ntasks={1064 if not is_hpc else 14} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
            body += f'run_rwd "$app" "$output_file" "$manifest" "$app_name" 1064 "$manifest_name" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\n'
        write(f"experiment/horeka/strong/rwd/{comp}_{s_name}.sh", header + body)
