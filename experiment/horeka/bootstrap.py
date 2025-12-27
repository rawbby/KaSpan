import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(os.path.dirname(SCRIPT_DIR))

APPS = {'kaspan': 'bench_kaspan', 'ispan': 'bench_ispan', 'hpc_graph': 'bench_hpc_graph'}

WEAK_LOCAL_N = [150000, 300000, 600000]
WEAK_D = [90, 100, 200, 400]
STRONG_N = [1000000000, 100000000, 10000000]

CONFIGS = {
    'single': {
        'nodes': 1, 'ntasks': 76, 'cpus': 1, 'socket': 38, 'node_tasks': 76,
        'hpc': {'ntasks': 1, 'socket': 1, 'node_tasks': 1, 'cpus': 76}
    },
    'medium': {
        'nodes': 7, 'ntasks': 532, 'cpus': 1, 'socket': 38, 'node_tasks': 76,
        'hpc': {'ntasks': 7, 'socket': 1, 'node_tasks': 1, 'cpus': 76}
    },
    '1064': {
        'nodes': 14, 'ntasks': 1064, 'cpus': 1, 'socket': 38, 'node_tasks': 76,
        'hpc': {'ntasks': 14, 'socket': 1, 'node_tasks': 1, 'cpus': 76}
    },
    '2052': {
        'nodes': 27, 'ntasks': 2052, 'cpus': 1, 'socket': 38, 'node_tasks': 76,
        'hpc': {'ntasks': 27, 'socket': 1, 'node_tasks': 1, 'cpus': 76}
    },
    '4104': {
        'nodes': 54, 'ntasks': 4104, 'cpus': 1, 'socket': 38, 'node_tasks': 76,
        'hpc': {'ntasks': 54, 'socket': 1, 'node_tasks': 1, 'cpus': 76}
    },
}
GENERICS = {
    'gnm-directed': 'gnm-directed;n=${n};m=${m};seed=13',
    'rmat_a59_bc19': 'rmat;directed;n=${n};m=${m};a=0.59;b=0.19;c=0.19;seed=13',
    'rmat_abc25': 'rmat;directed;n=${n};m=${m};a=0.25;b=0.25;c=0.25;seed=13'
}

def write_file(path, content):
    abs_path = os.path.join(SCRIPT_DIR, path)
    os.makedirs(os.path.dirname(abs_path), exist_ok=True)
    with open(abs_path, 'w') as f:
        f.write(content)

def get_header(comp, s_name, scaling, is_rwd=False):
    is_hpc = comp == 'hpc_graph'
    cfg = CONFIGS[s_name]
    c = cfg['hpc'] if is_hpc else cfg
    app_bin = APPS[comp]

    if is_rwd:
        out_err_name = f"{comp}_{s_name}"
        job_name = out_err_name
        run_script = "run_rwd.sh"
    else:
        sbatch_name = 'kaspan' if comp == 'ispan' and scaling == 'weak' else comp
        out_err_name = f"{sbatch_name}_{s_name}"
        if comp == 'ispan' and scaling == 'strong' and s_name == 'single':
            out_err_name = "isapn_single"
        job_name = 'ispan_single' if comp == 'ispan' and scaling == 'strong' and s_name == 'single' else out_err_name
        run_script = "run_generic.sh"

    header = f"""#!/bin/bash
#SBATCH --nodes={cfg['nodes']}
#SBATCH --ntasks={c['ntasks']}
#SBATCH --cpus-per-task={c['cpus']}
#SBATCH --ntasks-per-socket={c['socket']}
#SBATCH --ntasks-per-node={c['node_tasks']}
#SBATCH -o {out_err_name}.out
#SBATCH -e {out_err_name}.err
#SBATCH -J {job_name}
#SBATCH --partition=cpuonly
#SBATCH --time=30:00
#SBATCH --export=ALL
set -euo pipefail
source {SCRIPT_DIR}/env.sh
source {SCRIPT_DIR}/{run_script}"""
    if s_name == 'medium':
        header += f"\nsource {SCRIPT_DIR}/run_parallel.sh"
    header += f"\napp_name={comp}\napp={PROJECT_ROOT}/cmake-build-release/bin/{app_bin}"
    if is_rwd:
        header += f"\nrwd=( {SCRIPT_DIR}/rwd/*.manifest )"
    header += "\nset +eu"
    return header

def get_generic_body(comp, s_name, scaling, g_name, kagen):
    is_hpc = comp == 'hpc_graph'
    cfg = CONFIGS[s_name]
    local_n_list = " ".join(map(str, WEAK_LOCAL_N))
    d_list = " ".join(map(str, WEAK_D))
    n_list = " ".join(map(str, STRONG_N))

    if scaling == 'weak':
        if s_name == 'single':
            body = f"\nfor np in 76 38 16 8 4 2 1; do\nfor local_n in {local_n_list}; do\nfor d in {d_list}; do\nn=$(( np * local_n ))\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\noutput_file=\"${{app_name}}_{g_name}_np${{np}}_n${{local_n}}_d${{d}}.json\"\n"
            run_args = "--nodes=1 --ntasks=$np --cpus-per-task=1 --hint=nomultithread" if not is_hpc else "--nodes=1 --ntasks=1 --ntasks-per-socket=1 --cpus-per-task=$np"
            body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$local_n" "$d" "{g_name}" "{run_args}"{" --threads $np" if is_hpc else ""}\ndone\ndone\ndone\n'
        elif s_name == 'medium':
            body = f"\nfor local_n in {local_n_list}; do\nfor d in {d_list}; do\n"
            for np_val, nodes_val in [(152, 2), (304, 4)]:
                body += f"n=$(( {np_val} * local_n ))\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\n"
                run_args = f'--nodes={nodes_val} --ntasks={np_val if not is_hpc else nodes_val} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                body += f'run_async run_generic "$app" "${{app_name}}_{g_name}_np{np_val}_n${{local_n}}_d${{d}}.json" "$kagen_string" "$app_name" {np_val} "$local_n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\n'
            body += f"wait_all\ndone\ndone\n# np=532\nfor local_n in {local_n_list}; do\nfor d in {d_list}; do\nn=$(( 532 * local_n ))\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\n"
            run_args = f'--nodes=7 --ntasks={532 if not is_hpc else 7} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
            body += f'run_generic "$app" "${{app_name}}_{g_name}_np532_n${{local_n}}_d${{d}}.json" "$kagen_string" "$app_name" 532 "$local_n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
        else:
            body = f"\nnp={cfg['ntasks']}\nfor local_n in {local_n_list}; do\nfor d in {d_list}; do\nn=$(( np * local_n ))\n"
            m_line, kg = "m=$(( n * d / 100 ))", kagen
            if is_hpc and s_name == '1064' and g_name == 'gnm-directed':
                m_line, kg = "m=$(( local_n * d / 100 ))", "gnm-directed;n=${n};m=${m};seed=13"
            body += f"{m_line}\nkagen_string=\"{kg}\"\noutput_file=\"${{app_name}}_{g_name}_np${{np}}_n${{local_n}}_d${{d}}.json\"\n"
            run_args = f'--nodes={cfg["nodes"]} --ntasks={cfg["ntasks"] if not is_hpc else cfg["nodes"]} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
            body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$local_n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
    else:  # strong
        if s_name == 'single':
            body = f"\nfor np in 76 38 16 8 4 2 1; do\nfor n in {n_list}; do\nfor d in {d_list}; do\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\noutput_file=\"${{app_name}}_{g_name}_np${{np}}_n${{n}}_d${{d}}.json\"\n"
            run_args = "--nodes=1 --ntasks=$np --cpus-per-task=1 --hint=nomultithread" if not is_hpc else "--nodes=1 --ntasks=1 --ntasks-per-socket=1 --cpus-per-task=$np"
            body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$n" "$d" "{g_name}" "{run_args}"{" --threads $np" if is_hpc else ""}\ndone\ndone\ndone\n'
        elif s_name == 'medium':
            body = f"\nfor n in {n_list}; do\nfor d in {d_list}; do\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\n"
            for np_val, nodes_val in [(152, 2), (304, 4)]:
                run_args = f'--nodes={nodes_val} --ntasks={np_val if not is_hpc else nodes_val} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
                body += f'run_async run_generic "$app" "${{app_name}}_{g_name}_np{np_val}_n${{n}}_d${{d}}.json" "$kagen_string" "$app_name" {np_val} "$n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\n'
            body += f"wait_all\ndone\ndone\n# np=532\nfor n in {n_list}; do\nfor d in {d_list}; do\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\n"
            run_args = f'--nodes=7 --ntasks={532 if not is_hpc else 7} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
            body += f'run_generic "$app" "${{app_name}}_{g_name}_np532_n${{n}}_d${{d}}.json" "$kagen_string" "$app_name" 532 "$n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
        else:  # 1064, 2052, 4104
            body = f"\nnp={cfg['ntasks']}\nfor n in {n_list}; do\nfor d in {d_list}; do\nm=$(( n * d / 100 ))\nkagen_string=\"{kagen}\"\noutput_file=\"${{app_name}}_{g_name}_np${{np}}_n${{n}}_d${{d}}.json\"\n"
            n_run = cfg['ntasks'] if not is_hpc else cfg['nodes']
            run_args = f'--nodes={cfg["nodes"]} --ntasks={n_run} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
            body += f'run_generic "$app" "$output_file" "$kagen_string" "$app_name" "$np" "$n" "$d" "{g_name}" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\ndone\n'
    return body

def get_rwd_body(comp, s_name):
    is_hpc = comp == 'hpc_graph'
    cfg = CONFIGS[s_name]
    body = '\nfor manifest in "${rwd[@]}"; do\n'
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
    else:  # 1064, 2052, 4104
        np_val = cfg['ntasks']
        body += f"manifest_name=\"$(basename \"${{manifest%.manifest}}\")\"\noutput_file=\"${{app_name}}_${{manifest_name}}_np{np_val}.json\"\n"
        run_args = f'--nodes={cfg["nodes"]} --ntasks={np_val if not is_hpc else cfg["nodes"]} --cpus-per-task={1 if not is_hpc else 76}{" --hint=nomultithread" if not is_hpc else ""}'
        body += f'run_rwd "$app" "$output_file" "$manifest" "$app_name" {np_val} "$manifest_name" "{run_args}"{" --threads 76" if is_hpc else ""}\ndone\n'
    return body

def generate_generic_scaling(scaling):
    for g_name, kagen in GENERICS.items():
        for comp in APPS:
            for s_name in CONFIGS:
                header = get_header(comp, s_name, scaling)
                body = get_generic_body(comp, s_name, scaling, g_name, kagen)
                write_file(f"{scaling}/generic/{g_name}/{comp}_{s_name}.sh", header + body)

def generate_rwd_experiments():
    for comp in APPS:
        for s_name in CONFIGS:
            header = get_header(comp, s_name, scaling='strong', is_rwd=True)
            body = get_rwd_body(comp, s_name)
            write_file(f"strong/rwd/{comp}_{s_name}.sh", header + body)

if __name__ == '__main__':
    generate_generic_scaling('weak')
    generate_generic_scaling('strong')
    generate_rwd_experiments()
