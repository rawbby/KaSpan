import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../.."))

APPS = {'kaspan': 'bench_kaspan', 'ispan': 'bench_ispan', 'hpc_graph': 'bench_hpc_graph'}

WEAK_LOCAL_N = [150000, 300000, 600000]
WEAK_D = [90, 100, 200, 400, 1600]
STRONG_N = [100000000, 10000000, 1000000]

CONFIGS = {
    'local': {
        'nps': [1, 3, 6, 10]
    }
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


def get_header(comp, scaling, rel_path, is_rwd=False):
    app_bin = APPS[comp]

    header = f"""#!/bin/bash
set -euo pipefail
source {SCRIPT_DIR}/run_utils.sh
app_name={comp}
app={PROJECT_ROOT}/cmake-build-release/bin/{app_bin}
scaling={scaling}
"""
    if is_rwd:
        header += f"rwd_dir={PROJECT_ROOT}/experiment/rwd\n"
        header += "shopt -s nullglob\nrwd=( \"$rwd_dir\"/*.manifest )\nshopt -u nullglob\n"
    header += "set +eu\n"
    return header


def get_generic_body(comp, scaling, g_name, kagen):
    nps = CONFIGS['local']['nps']
    local_n_list = " ".join(map(str, WEAK_LOCAL_N))
    d_list = " ".join(map(str, WEAK_D))
    n_list = " ".join(map(str, STRONG_N))

    body = ""
    if scaling == 'weak':
        body += f"for np in {' '.join(map(str, nps))}; do\n"
        body += f"for local_n in {local_n_list}; do\n"
        body += f"for d in {d_list}; do\n"
        body += "n=$(( np * local_n ))\n"
        body += "m=$(( n * d / 100 ))\n"
        body += f"kagen_string=\"{kagen}\"\n"
        body += "output_file=\"${scaling}_${app_name}_np${np}_" + g_name + "_localn${local_n}_d${d}.json\"\n"
        body += "run_generic \"$app\" \"$output_file\" \"$kagen_string\" \"$app_name\" \"$np\" \"$n\" \"$d\" \"" + g_name + "\"\n"
        body += "done\ndone\ndone\n"
    else:  # strong
        body += f"for np in {' '.join(map(str, nps))}; do\n"
        body += f"for n in {n_list}; do\n"
        body += f"for d in {d_list}; do\n"
        body += "m=$(( n * d / 100 ))\n"
        body += f"kagen_string=\"{kagen}\"\n"
        body += "output_file=\"${scaling}_${app_name}_np${np}_" + g_name + "_n${n}_d${d}.json\"\n"
        body += "run_generic \"$app\" \"$output_file\" \"$kagen_string\" \"$app_name\" \"$np\" \"$n\" \"$d\" \"" + g_name + "\"\n"
        body += "done\ndone\ndone\n"
    return body


def get_rwd_body(comp):
    nps = CONFIGS['local']['nps']
    body = 'for manifest in "${rwd[@]}"; do\n'
    body += f"for np in {' '.join(map(str, nps))}; do\n"
    body += "manifest_name=\"$(basename \"${manifest%.manifest}\")\"\n"
    body += "output_file=\"${scaling}_${app_name}_np${np}_${manifest_name}.json\"\n"
    body += "run_rwd \"$app\" \"$output_file\" \"$manifest\" \"$app_name\" \"$np\" \"$manifest_name\"\n"
    body += "done\ndone\n"
    return body


def generate_generic_scaling(scaling):
    for g_name, kagen in GENERICS.items():
        for comp in APPS:
            rel_path = f"{scaling}/{g_name}/{comp}.sh"
            header = get_header(comp, scaling, rel_path)
            body = get_generic_body(comp, scaling, g_name, kagen)
            write_file(rel_path, header + body)


def generate_rwd_experiments():
    for comp in APPS:
        rel_path = f"strong/rwd/{comp}.sh"
        header = get_header(comp, scaling='strong', rel_path=rel_path, is_rwd=True)
        body = get_rwd_body(comp)
        write_file(rel_path, header + body)


if __name__ == '__main__':
    generate_generic_scaling('weak')
    generate_generic_scaling('strong')
    generate_rwd_experiments()
