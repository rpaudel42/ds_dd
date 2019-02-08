from graph.dataset import Dataset
from simulation.stream_generator import StreamGenerator
from dsdd.drift_detector import DriftDetector
from results.measure_performance import MeasurePerformance
from properties import Experiment

def main():
    results = {}
    ds = Dataset()

    #prepare datasets
    available_dataset = ds.get_available_dataset()
    for dataset in available_dataset:

        #print("Subgraph List: ", dataset.subgraph_list)

        param_n = dataset.param[0]
        param_w = dataset.param[1]

        print("\n\n----- Experimenting on Dataset[ %s ]-- param [ n = %s, w = %s] ----"%(dataset.dataset_name, param_n, param_w))
        sg = StreamGenerator()
        g_list = sg.get_graph_stream(dataset.file_name)

        for i in range(1, Experiment.iterations+1):
            print("\n----- Iteration [ ", i," ] -------\n")

            dd = DriftDetector()
            PE, E, DRIFT, FA = dd.drift_detector(g_list, dataset, param_n, param_w)
            del dd

            print("\nEntropy: ", E)

            print("\n----- Iteration [ ", i ," ] Ends ----\n\n")

            mp = MeasurePerformance()
            results[i] = mp.calculate_metrics(DRIFT, FA, dataset)
            mp.print_results(results[i], DRIFT, FA)
            del mp
            if i %10 == 0:
                mp = MeasurePerformance()
                summary = mp.aggregate_result(results)
                mp.print_sumary(summary, dataset, param_n, param_w, i)
                del mp

        mp = MeasurePerformance()
        summary = mp.aggregate_result(results)
        mp.print_sumary(summary, dataset, param_n, param_w, 50)
        del mp
    del ds

main()
