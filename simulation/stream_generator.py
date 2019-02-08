# ******************************************************************************
# streamGenerator.py
#
# Drift Detector, Detect drift point in Graph Stream
# Generate Synthetic Graph Stream
#
# Date      Name       Description
# ========  =========  ========================================================
# 03/20/2018  Paudel     Initial version,
#
# ******************************************************************************

import os
from properties import Experiment, RULSIF, SubGen

class StreamGenerator:
    #p = publisher()
    def __init__(self):
        print("\n\n----- Generating Graph Streams ----")

    def get_graph_stream(self, fileName):

        '''PURPOSE: Read .G Graph, load each XP as JSON and send each XP as a graph stream
                :param fileName:
                :return:
        '''
        graph = {}
        node = {}
        edge = {}
        g_list = {}
        XP = 0
        label = 'pos'
        with open(fileName) as f:
            lines = f.readlines()
            for line in lines:
                singles = line.split(' ')
                if singles[0] == "XP" or singles[0] == "XN":
                    if singles[0] == "XP":
                        label = 'pos'
                    if singles[0] == "XN":
                        label = 'neg'
                    if XP > 0:
                        graph["node"] = node
                        graph["edge"] = edge
                        graph["label"] = label
                        g_list[XP] = graph

                    graph = {}
                    node = {}
                    edge = {}
                    XP += 1
                elif singles[0] == "v":
                    node[singles[1]] = singles[2].strip('\n').strip('\"')
                elif (singles[0] == "u" or singles[0] == "d"):
                    edge[singles[1] + ' ' + singles[2]] = singles[3].strip('\n').strip('\"')
        return g_list


    def read_send_nel_file(self, fileName):

        '''PURPOSE: Read .G Graph, load each XP as JSON and send each XP as a graph stream
                :param fileName:
                :return:
        '''
        g_list = {}
        graph = {}
        node = {}
        edge = {}
        XP = 0
        label = 'pos'

        with open(fileName) as f:
            lines = f.readlines()
            for line in lines:
                singles = line.split(' ')
                if singles[0] == "x":
                    graph["node"] = node
                    graph["edge"] = edge
                    graph["label"] = label
                    if singles[1].strip('\n') == "1.0" and XP <= 700:
                        g_list[XP] = graph
                        #print(graph)
                        #self.p.sendMessage(json.dumps(graph))
                        #print("[", str(XP), "] Graph Sent")
                    #if XP > 2 and XP % GraphEntropy.windowSize == 1:
                    #    time.sleep(1000)
                    graph = {}
                    node = {}
                    edge = {}
                    label = 'pos'
                    XP += 1
                elif singles[0] == "n":
                    node[int(singles[1])] = singles[2].strip('\n')
                elif singles[0] == "e":
                    edge[singles[1] + ' ' + singles[2]] =  singles[3].strip('\n')
        return g_list

