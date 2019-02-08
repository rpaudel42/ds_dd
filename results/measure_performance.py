import math
from properties import RULSIF as ru, Experiment as gs
from pylab import *
import datetime

class MeasurePerformance:
    def __init__(self):
        print('---- Calculating Performance ----')

    def calculate_metrics(self, detected_drift_points, false_alarm, dataset):
        results = {}

        accepted_delay = (2*ru.n+ru.k)
        try:
            # Calculate f_a1000
            try:
                f_a1000 = (len(false_alarm) / dataset.total_graphs) * 1000
            except ValueError as e:
                f_a1000 = 0
                print("Error in FA Calculation: ", e)

            results['f_a1000'] = f_a1000

            #Calculate DCR
            n_cr = len(detected_drift_points)             #of times correct change points detected (True Alarm)
            n_cp = len(dataset.drift_points)               #of all change points
            try:
                dcr = n_cr/n_cp
            except ValueError as e:
                dcr = 0
                print("Error in DcR Calculation: ", e)

            results['dcr'] = dcr

            #Calculate detection_delay
            delay = 0
            detection_delay = 0
            try:
                if len(detected_drift_points)>0:
                    for i in dataset.drift_points:
                        for j in detected_drift_points:
                            #print("I: ", i , " J: ", j)
                            if i <= j <= (i + accepted_delay):
                                delay += (j-i)
                                #print("Delay: ", delay)
                    detection_delay = delay / len(detected_drift_points)
            except ValueError as e:
                detection_delay = 0
                print("Error in detection_delay Calculation: ", e)

            results['detection_delay'] = detection_delay
            return results
        except IndexError as e:
            print("Error Calculating Metrices: ", e)
            return None



    def print_results(self, results, DRIFT, FA):

        print("\n ------  RESULTS ------")
        print("Drift Points: ", DRIFT)
        print("False Alarm: ", FA)

        print("\n ------  Score ------")
        print("DoD:-          ", results['detection_delay'])
        print("FA1000:-       ", results['f_a1000'])
        print("DCR:-          ", results['dcr'])


    def aggregate_result(self, results):
        summary = {}
        f_a1000 = []
        dcr = []
        detection_delay = []

        for i in results:
            f_a1000.append(results[i]['f_a1000'])
            dcr.append(results[i]['dcr'])
            detection_delay.append(results[i]['detection_delay'])

        summary['f_a1000'] = [array(f_a1000).mean(), array(f_a1000).std()]
        summary['dcr'] = [array(dcr).mean(), array(dcr).std()] #AR / len(results)
        summary['detection_delay'] = [array(detection_delay).mean(), array(detection_delay).std()] #DoD / len(results)

        return summary

    def print_sumary(self, summary, dataset, param_n, param_w, run):
        file_name = dataset.dataset_name+'_results.txt'

        result_text = '\n\n\n ------ Final Result  [ '+dataset.dataset_name + ' Run ' + str(run) + ' -- param[ n = '+ str(param_n) + ', w = ' + str(param_w) +'] ]-----\n'
        result_text += 'DoD:-         mean [ ' + str(summary['detection_delay'][0]) + ',  ] std [  ' + str(summary['detection_delay'][1]) + ' ]\n'
        result_text += 'FA1000:-      mean [ ' + str(summary['f_a1000'][0]) + ' ] std [ '+ str(summary['f_a1000'][1])+' ]\n'
        result_text += 'DCR:-         mean [ ' + str(summary['dcr'][0]) + ' ] std [ '+ str(summary['dcr'][1])+ ' ]\n'
        try:
            fw = open(file_name, 'a')
            fw.write(result_text)
            print(result_text)
        except Exception as e:
            print("\nCannot write results: ", e)
            print(result_text)
            pass
