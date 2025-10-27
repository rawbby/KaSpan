import datetime
import math
import random
import subprocess
from itertools import product
from pathlib import Path
from typing import List, Dict, Any

__all__ = [
    'script_dir',
    'project_dir',
    'build_dir',
    'binary_dir',
    'experiment_data_dir',
    'random_seed',
    'random_seeds',
    'seed_hash',
    'now_hash',
    'get_git_hash',
    'matrix_product'
]

script_dir = Path(__file__).parent.absolute()
project_dir = script_dir.parent
build_dir = project_dir / 'cmake-build-release'
binary_dir = build_dir / 'bin'
experiment_data_dir = build_dir / 'experiment_data'


def random_seed() -> str:
    return str(random.randint(0, 99999999))


def random_seeds(n: int) -> List[str]:
    return [random_seed() for _ in range(n)]


def seed_hash(length: int = 2) -> str:
    return random.randint(0, (256 ** length) - 1).to_bytes(length=length, signed=False).hex()


def now_hash(length: int = 3) -> str:
    now = datetime.datetime.now()
    posix_timestamp = math.floor(now.timestamp())
    truncated_posix_timestamp = posix_timestamp % (256 ** length)
    return truncated_posix_timestamp.to_bytes(length=length, signed=False).hex()


def get_git_hash():
    out = subprocess.check_output(['git', '-C', project_dir, 'rev-parse'], stderr=subprocess.DEVNULL, text=True)
    return out.strip()


def matrix_product(matrix: Dict[str, List[Any]], *keys):
    return product(*[matrix[k] for k in keys])

# def get_experiment_job_names(experiment_name: Optional[str] = None) -> Union[Dict[str, List[str]], List[str]]:
#     def get_experiment_job_names_impl(experiment_dir: Path):
#         assert experiment_dir.is_dir()
#         job_names: Set[str] = set()
#         for job_file_path in Path(experiment_dir).iterdir():
#             if not job_file_path.is_file():
#                 continue
#             if not job_file_path.name.startswith(job_file_prefix):
#                 continue
#             if not job_file_path.name.endswith(job_file_extension):
#                 continue
#             job_names.add(job_file_path.name[len(job_file_prefix): -len(job_file_extension)])
#         return sorted(job_names)
#
#     if experiment_name is None:
#         experiment_job_names: Dict[str, List[str]] = dict()
#         for experiment_dir_path in experiment_data_dir.iterdir():
#             if not experiment_dir_path.is_dir():
#                 continue
#             experiment_name = experiment_dir_path.name
#             experiment_job_names[experiment_name] = get_experiment_job_names_impl(experiment_dir_path)
#         return experiment_job_names
#
#     return get_experiment_job_names_impl(get_experiment_dir(experiment_name))
#
#
# def get_slurm_experiment_job_names(experiment_name: Optional[str] = None) -> Union[Dict[str, List[str]], List[str]]:
#     result = subprocess.run(['squeue', '-h', '-O', 'Name:128'],
#                             stdout=subprocess.PIPE,
#                             stderr=subprocess.DEVNULL,
#                             text=True, check=True)
#
#     if experiment_name is None:
#         experiment_jobs: Dict[str, List[str]] = dict()
#         for line in result.stdout.splitlines():
#             experiment_name, job_name = split_slurm_job_name(line.strip())
#             (experiment_jobs
#              .setdefault(experiment_name, list())
#              .append(job_name))
#
#     else:
#         experiment_jobs: List[str] = list()
#         for line in result.stdout.splitlines():
#             experiment_name_, job_name = split_slurm_job_name(line.strip())
#             if experiment_name == experiment_name_:
#                 experiment_jobs.append(job_name)
#
#     return experiment_jobs
#
#
# def classify_experiment_job_names(experiment_name: str, experiment_job_names: Optional[Iterable[str]] = None) -> Tuple[
#     List[str], List[str], List[str]]:
#     if experiment_job_names is None:
#         experiment_job_names: List[str] = get_experiment_job_names(experiment_name)
#
#     wip_job_names: List[str] = get_slurm_experiment_job_names(experiment_name)
#
#     done_job_names: List[str] = list()
#     todo_job_names: List[str] = list()
#
#     for experiment_job_name in experiment_job_names:
#         if experiment_job_name in wip_job_names:
#             continue
#
#         experiment_job_file_path = get_job_file_path(experiment_name, experiment_job_name)
#         assert experiment_job_file_path.is_file()
#
#         experiment_result_file_path = get_result_file_path(experiment_name, experiment_job_name)
#         if experiment_result_file_path.is_file() and experiment_result_file_path.stat().st_size > 0:
#             done_job_names.append(experiment_job_name)
#         else:
#             todo_job_names.append(experiment_job_name)
#
#     return todo_job_names, wip_job_names, done_job_names
