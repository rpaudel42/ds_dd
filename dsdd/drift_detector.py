# ******************************************************************************
# drift_detector.py
#
# Drift Detector, Detect drift point in Graph Stream
#
#
# Date      Name       Description
# ========  =========  ========================================================
# 03/20/2018  Paudel     Initial version,
# ******************************************************************************
#

from pylab import *

import os
import networkx as nx
import math
from random import shuffle
from rulsif.change_detection import ChangeDetection
from properties import RULSIF, Experiment, GBAD

class DriftDetector:
    is_isomorphic = False
    subgraph_id = 1
    S_w = {}


    def __init__(self):
        # remove graph file if exist
        #print("Subgraph_id: ", self.subgraph_id)
        #print("S_w: ", self.S_w)
        self.S_w.clear()
        self.is_isomorphic = False
        self.subgraph_id = 1
        #print("\n\n\nClear S_w: ", self.S_w, self.subgraph_id, self.is_isomorphic)
        self.subgraph_id = 1
        print("Starting Drift Detection-----")

        try:
            os.remove(GBAD.graph_folder + "/G.g")
            os.remove(GBAD.graph_folder + "/subgraph.g")
        except OSError:
            pass


    @staticmethod
    def get_subgraph_count(subgraph):
        '''
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        # NAME: getTotalSubgraphCount
        #
        # INPUTS: (subgraph) List of subgraph in this window
        #
        # RETURN: (total) count of total subgraph
        #
        # PURPOSE: Get the total count of subgraph in current window  for calculating Entropy
        #
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        :param subgraph:
        :return:
        '''
        s_count = 0
        for s in subgraph:
            s_count += s[0]
        return s_count


    @staticmethod
    def get_total_count():
        '''
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        # NAME: getTotalCount
        #
        # INPUTS: ()
        #
        # RETURN: (total, pos, neg) Count of total subgraph, instances of positive and negative subgraph
        #
        # PURPOSE: Get the total count of subgraph in a multiple window considered for calculating Entropy
        #
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        :return:
        '''
        total = 0
        for id in DriftDetector.S_w.keys():
            #sg_list = DriftDetector.subgraph_list[id]
            #print ("ID List: ", DriftDetector.subgraph_list[id])
            if len(DriftDetector.S_w[id][1:]) > 1:
                for list in DriftDetector.S_w[id][1:]:
                    # print(" inner list: ", list)
                    total += int(list[0])
                    #print(total)
        return total


    @staticmethod
    def get_window_entropy():
        '''
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        # NAME: getWindowEntropy
        #
        # INPUTS: (NONE)
        #
        # RETURN: (entropy)
        #
        # PURPOSE: Calculate the entropy of current window by using subgraph from current window
        #
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **

        :param subgraph:
        :return:
        '''
        eW = 0
        total = DriftDetector.get_total_count()
        #print("Total: ", total)
        for id in list(DriftDetector.S_w.keys()):
            #If subgraph is only present in one Graph (Gi) then entropy is zero
            if len(DriftDetector.S_w[id][1:]) > 1:
                #print("S List: ", DriftDetector.S_w[id])
                eSi = 0
                pS = 0
                s_total = DriftDetector.get_subgraph_count(DriftDetector.S_w[id][1:])
                #print("S total: ", s_total)
                for S in DriftDetector.S_w[id][1:]:
                    #print("S: ", S[0], "S_ Total: ", s_total)
                    PsGi = S[0] / s_total
                    eSi += PsGi * math.log2(PsGi)

                #print("Esi: ", eSi)
                pS = s_total/total
                eW += -1 * pS * eSi
        #print("Entropy of Window: ", eW)
        return eW


    '''
    @staticmethod
    def save_graph_file(message, graphCount, param_n, param_w, dataset_name):

        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        # NAME: saveGBADGraphFile()
        #
        # INPUTS: (message) Json message stream received from sender
        #
        # RETURN: (fileName)
        #
        # PURPOSE: Save the received JSON Graph file as a GBAD format graph
        #
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **

        #:param message:
        #:return:

        fileName = GBAD.graph_folder + "/" +dataset_name + '_' + str(param_n) + '_' + str(param_w) + '.g'
        fw = open(fileName, "w")
        fw.write("XP # 1 //" + str(graphCount) + "\n")
        for key in message['node']:
            fw.write("v " + key + " \"" + message['node'][key] + "\"\n")
        for key in message['edge']:
            k = key.split(' ', 1)
            if message['edge'][key]:
                fw.write("d " + k[0] + " " + k[1] + " \"" + message['edge'][key] + "\"\n")
            else:
                fw.write("d " + k[0] + " " + k[1] + "\n")

        return fileName

    @staticmethod
    def read_subgraph(subgraphFile):
        # This is to read in networkx format
        s = {}
        instance = 0
        with open(subgraphFile) as f:
            lines = f.readlines()
            for l in lines:
                item = l.split(' ')
                if l == "\n":
                    s[sub_graph] = instance  # insert total instance count, new window and old window..  old window 0 this time
                elif item[0] == 'S':
                    sub_graph = nx.DiGraph()
                    instance = int(item[1].strip('\n').strip(' '))
                elif item[0] == 'v':
                    sub_graph.add_node(item[1], label=item[2].strip('\n').strip('\"'))
                elif item[0] == 'u' or item[0] == 'd':
                    sub_graph.add_edge(item[1], item[2], label=item[3].strip('\n').strip('\"'))
        return s

    @staticmethod
    def get_discriminative_subgraph(graph_file, param_n, param_w, dataset_name):
        subgraph_file = GBAD.graph_folder + "/" + "SG_" + dataset_name + '_' + str(param_n) + '_' + str(param_w) + '.g'
        output_file = GBAD.graph_folder + "/" + "out_" + dataset_name + '_' + str(param_n) + '_' + str(param_w) + '.txt'
        try:
            os.remove(subgraph_file)
            os.remove(output_file)
        except OSError:
            pass

        command = GBAD.gbad_home + "/" + GBAD.run_command + " -nsubs " + str(param_n)+ " -out " + subgraph_file + " " + graph_file + ">>"+output_file

        #print(command)
        os.system(command)
        #print("Finish GBAD")
        return DriftDetector.read_subgraph(subgraph_file)
    '''

    @staticmethod
    def match_edge(G1, G2):
        #print("Inside match edge")
        match = []
        if len(list(G1.edges(data=True))) == len(list(G2.edges(data=True))):
            for edge in list(G1.edges(data=True)):
                match.append(False)
            index = 0
            for i, e1 in enumerate(G1.edges(data=True)):
                for j, e2 in enumerate(G2.edges(data=True)):
                    if e1[2]['label'] == e2[2]['label']:
                        if (e1[0] == e2[0] or e1[0] == e2[1]):
                            if (e1[1] == e2[0] or e1[1] == e2[1]):
                                match[i] = True
                                break
            # print(set(match))
            if set(match) == set([True]):
                #print("Match Edges", )
                #print(G1.edges(data=True))
                #print(G2.edges(data=True))
                return True
        return False

    @staticmethod
    def match_node(G1, G2):
        match = []
        if len(list(G1.nodes(data=True))) == len(list(G2.nodes(data=True))):
            for node in list(G1.nodes(data=True)):
                match.append(False)
            index = 0
            for i, n1 in enumerate(G1.nodes(data=True)):
                for j, n2 in enumerate(G2.nodes(data=True)):
                    #print("Node G1 :", n1, "\nNode G2: ", n2)
                    if n1[1]['label'] == n2[1]['label']:
                        if n1[0] == n2[0]:
                            match[i] = True
                            break
            # print(set(match))
            if set(match) == set([True]):
                #print("Match Nodes: \n", )
                #print(G1.nodes(data=True))
                #print(G2.nodes(data=True))
                return True
        return False

    @staticmethod
    def update_subgraph_window(s, graphCount, param_w):
        '''
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        # NAME: addWindowSubGraphToEntropySubGraphList()
        #
        # INPUTS: (s) Subgraph List with count of instances for current window
        #
        # RETURN: ()
        #
        # PURPOSE: Collect unique subgraph from multiple window into single list, isomorphic graph count are merged together
        #
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **

        :param s:
        :return:
        '''
        #s_win = graphCount #GraphEntropyMethod.winCount
        #print("S_w: ", DriftDetector.S_w)
        # remove all subgraph from oldest window
        for id in list(DriftDetector.S_w.keys()):
            #print("I:  -- ", DriftDetector.S_w[id][1:])
            #Find Frequency and window number
            filters = list(
                filter(lambda x: param_w <= graphCount - x[1],
                       DriftDetector.S_w[id][1:]))

            #Remove frequency entry from selected filters
            DriftDetector.S_w[id] =  [x for x in DriftDetector.S_w[id] if x not in filters]

            if len(DriftDetector.S_w[id]) == 1: #if it has only subgraph but no frequency
                #print("When is it here?")
                del DriftDetector.S_w[id]

        #print("After Deletion S_w: ", DriftDetector.S_w)
        # add all subgraph to entropy subgraph list till subgraph list is full
        count_array = []
        for sg in s.keys():
            s1 = nx.DiGraph(sg)
            #print("New Subgraph: ", s1.nodes())
            if len(DriftDetector.S_w) > 0:
                #print("Exisiting SW")
                #print("List: ", DriftDetector.S_w)
                for id in list(DriftDetector.S_w.keys()):
                    #print("Sg: ", GraphEntropyMethod.subgraph_list[id])
                    #print("Sg Graph: ", GraphEntropyMethod.subgraph_list[id][0])
                    #for isomorphic_graph_info in GraphEntropyMethod.subgraph_list[id]:
                    if 1 == 1:
                        s2 = nx.DiGraph(DriftDetector.S_w[id][0])
                        # GM = isomorphism.GraphMatcher(s1, s2)
                        # if GM.is_isomorphic():
                        #key = list(d.keys())[0]
                        #print("S2 Node: ", s2.nodes(), " S2 Edges: ", s2.edges())
                        if DriftDetector.match_node(s1, s2) and DriftDetector.match_edge(s1, s2):
                            DriftDetector.is_isomorphic = True
                            #print("Isomorphic Graphs--", id, "\nNode S1  ", nx.get_node_attributes(s1, 'label'), "  \nNode s2  ", nx.get_node_attributes(s2, 'label'))
                            #print("Edge Match: ", GraphEntropyMethod.match_edge(s1, s2))

                            #print("\nEdges S1: ", nx.get_edge_attributes(s1, 'label'), "\nEdges S2: ", nx.get_edge_attributes(s2, 'label'))
                            DriftDetector.S_w[id].append([s[sg], graphCount])
                            break
                        else:
                            DriftDetector.is_isomorphic = False

                    if DriftDetector.is_isomorphic:
                        break

            #print("Check Isomorphic: ", DriftDetector.is_isomorphic)
            if len(DriftDetector.S_w) == 0:
                #print("Add First: ", s1.nodes())
                DriftDetector.subgraph_id += 1
                DriftDetector.S_w[DriftDetector.subgraph_id] = []
                DriftDetector.S_w[DriftDetector.subgraph_id].append(s1)
                DriftDetector.S_w[DriftDetector.subgraph_id].append([s[sg], graphCount])

            if DriftDetector.is_isomorphic is False:
                #print("Add First: ", s1.nodes())
                DriftDetector.subgraph_id += 1
                DriftDetector.S_w[DriftDetector.subgraph_id] = []
                DriftDetector.S_w[DriftDetector.subgraph_id].append(s1)
                DriftDetector.S_w[DriftDetector.subgraph_id].append([s[sg], graphCount])

        #print("Final Subgraph List: \n", DriftDetector.S_w)
        #print("-------------------")

    @staticmethod
    def get_change_score(E):
        #print("E: ", E, "Len: ", len(E))
        cd = ChangeDetection()

        #print("Entropy Array: ", DriftDetector.E[Tstart:Tend])
        WIN = cd.sliding_window(E, RULSIF.k, 1)
        #print("WIN: ", WIN)

        t = RULSIF.n + 1

        Y = WIN[:, arange(t - RULSIF.n - 1, RULSIF.n + t - 1)]
        YRef = Y[:, arange(0, RULSIF.n)]
        YTest = Y[:, arange(RULSIF.n, 2 * RULSIF.n)]

        (PE, w, s) = cd.R_ULSIF(YTest, YRef, Y, RULSIF.alpha, cd.sigma_list(YTest, YRef), cd.lambda_list(), YTest.shape[1], RULSIF.k_fold)

        return PE


    @staticmethod
    def is_real_drift(i, drift_points):
        for j in drift_points:
            if j <= i < (j + 2* RULSIF.n + RULSIF.k):
                return True
        return False

    @staticmethod
    def shuffule_graphs(g_id, dataset):
        b_key = []
        a_key = []
        for id in g_id:
            if id <= dataset.drift_points[0]:
                b_key.append(id)
            else:
                a_key.append(id)
        shuffle(b_key)
        shuffle(a_key)
        return (b_key + a_key)

    @staticmethod
    def set_dynamic_threshold(pe_alpha, param_w):
        if len(pe_alpha) > param_w:
            pe_w = array(pe_alpha[len(pe_alpha) - param_w:len(pe_alpha)])
            mean = pe_w.mean()
            std = pe_w.std()
            return (mean + std)
        else:
            return RULSIF.th


    @staticmethod
    def drift_detector(g_list, dataset, param_n, param_w):
        '''
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **
        # NAME: graphEntropyMethod()
        #
        # INPUTS: (g_list{}) Graph Stream
        #
        # RETURN: (PE, Entropy, Drift Points and False Alarm)
        #
        # PURPOSE: Implementation of GEM, that give drift point
        #
        # ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **

        :param g_list:
        :return:
        '''

        #subgraph list from all graphs

        DRIFT = []      #Detected Drift Points
        WARN = []
        FA = []         #False Alarm
        e = []          #entropy series
        PE = []
        t_buffer = 2 * RULSIF.n + RULSIF.k - 1
        t_next = 2 * RULSIF.n + RULSIF.k - 1

        key = DriftDetector.shuffule_graphs(list(g_list.keys()), dataset)
        t = 0
        for g_count in key:
            #print("Window Size: ", RULSIF.W)
            #print("Graph Count: ", g_count, " T: ", t)
            t += 1
            #graphFile = DriftDetector.save_graph_file(g_list[g_count], t, param_n, param_w, dataset.dataset_name)

            #get discriminative subgraph from the pre-populated list
            s = dataset.subgraph_list[g_count]
            #s = DriftDetector.get_discriminative_subgraph(graphFile, param_n, param_w, dataset.dataset_name)
            #print("\n \n \n G Count: ", g_count, " T: ", t)#, " \n S_w : ", DriftDetector.S_w)
            #update S_w,
            DriftDetector.update_subgraph_window(s, t, param_w)

            if t >= param_w:

                if t == param_w:
                    print("Window Size Reached")
                    print("Starting Entropy Calculation")

                i = (t - param_w)
                e.append(DriftDetector.get_window_entropy())


                if i >= t_buffer and i == t_next:

                    if i == t_buffer:
                        print("Buffer Matrix Full")
                        print("Calculating diversion score")

                    e_curr = e[i-t_buffer:i]
                    #print("E Curr: ", e_curr)
                    score = DriftDetector.get_change_score(e_curr)
                    PE.append(score)

                    th = DriftDetector.set_dynamic_threshold(PE, param_w)

                    #print("Threshold: ", th)
                    if score >= th:
                        #Removing duplicate alarm and recording False Alarm
                        if DriftDetector.is_real_drift(t, dataset.drift_points):
                            DRIFT.append(t)
                            t_next = i + (2 * RULSIF.n + RULSIF.k)  #other alaram till t_next are consider duplicate so skip them
                            print("Drift Detected at:  [ ", t, "  ]")
                        else:
                            WARN.append(t)
                            if len(WARN) == 1:
                                FA.append(t)
                                print("False Alarm at:  [ ", t, "  ] ")
                            else:
                                if WARN[-1] - WARN[-2] >= (2 * RULSIF.n + RULSIF.k): #remove duplicate False Alarm
                                    FA.append(t)
                                    print("False Alarm at:  [ ", t, "  ] ")
                            t_next = i + 1
                    else:
                        t_next = i + 1

                    #i += 1 # check if we need this

        #print("Final S_w", DriftDetector.S_w)
        return PE, e, DRIFT, FA



