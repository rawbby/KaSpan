import pathlib
import os
import plot_utils


def main():
    cwd = pathlib.Path(os.getcwd())
    scaling_data, all_metrics_raw, apps, all_stages, global_all_nps = plot_utils.load_all_data(cwd)

    if not scaling_data["strong"] and not scaling_data["weak"]:
        print("No scaling data found.")
        return

    global_min, global_max = plot_utils.calculate_global_stats(all_metrics_raw)
    app_style, stage_style = plot_utils.get_styles(apps, all_stages)
    stage_style["Other"] = ("gray", "x")

    # Plot Strong Scaling
    if scaling_data["strong"]:
        strong_data = scaling_data["strong"]
        for graph in sorted(list(strong_data.keys())):
            print(f"Plotting strong scaling for graph: {graph}")
            graph_data = strong_data[graph]
            plot_utils.plot_strong_summary(graph, graph_data, apps, global_min, global_max, app_style, cwd)
            plot_utils.plot_strong_steps(graph, graph_data, apps, global_min, global_max, stage_style, cwd)

    # Plot Weak Scaling
    if scaling_data["weak"]:
        weak_data = scaling_data["weak"]
        for graph in sorted(list(weak_data.keys())):
            print(f"Plotting weak scaling for graph: {graph}")
            graph_data = weak_data[graph]
            plot_utils.plot_weak_summary(graph, graph_data, apps, global_min, global_max, app_style, cwd)
            plot_utils.plot_weak_steps(graph, graph_data, apps, global_min, global_max, stage_style, cwd)


if __name__ == "__main__":
    main()
