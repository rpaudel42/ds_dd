# ******************************************************************************
# prepare_dataset.py
#
# Date      Name       Description
# ========  =========  ========================================================
# 1/27/19   Paudel     Initial version,
# ******************************************************************************
from properties import DataList, SubGen, GBAD
import os
import networkx as nx

class Dataset():
    dataset_name = None
    total_graphs = None
    drift_points = None
    file_name = None
    is_synthetic = False
    param = None
    available_dataset = []
    subgraph_list = {}

    def __int__(self):
        print("\n\n----- Preparing Dataset ----")
        pass


    def save_synthetic_graph(self, g_list, file_name):
        '''
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

        :param g_list:
        :return:
        '''
        #fileName = GBAD.graph_folder + "/" + GBAD.graph_file
        fw = open(file_name, "w")
        for i in g_list:
            if g_list[i]['label'] == 'pos':
                fw.write("XP # " + str(i) + "\n")
            else:
                fw.write("XN # " + str(i) + "\n")

            for key in g_list[i]['node']:
                fw.write("v " + key + " \"" + g_list[i]['node'][key] + "\"\n")
            for key in g_list[i]['edge']:
                k = key.split(' ', 1)
                if g_list[i]['edge'][key]:
                    fw.write("d " + k[0] + " " + k[1] + " \"" + g_list[i]['edge'][key] + "\"\n")
                else:
                    fw.write("d " + k[0] + " " + k[1] + "\n")
        #This is for reading ease
        fw.write("XP # " + str(i) + "\n")


    def read_graph(self, file_name, label):
        '''
        PURPOSE: Read Synthetic Graph and load as dictionary of a graph file
        :param file_name:
        :param label:
        :return:
        '''
        subdue_graph = {}
        graph = {}
        node = {}
        edge = {}
        with open(file_name) as f:
            lines = f.readlines()
        for l in lines:
            graph_entry = l.split(" ")
            if graph_entry[0] == 'v':
                node[graph_entry[1]] = graph_entry[2].strip('\n')
            elif graph_entry[0] == 'e':
                edge[graph_entry[2] + ' ' + graph_entry[3].strip('\n')] = graph_entry[1]
        graph["node"] = node
        graph["edge"] = edge
        graph["label"] = label
        return graph

    def create_synthetic_graph(self, graphSample, seed, label):
        '''
        PURPOSE: create synthetic graph
        :param graphSample:
        :param seed:
        :return:
        '''

        filename = graphSample + ".graph"
        try:
            if os.path.exists(filename):
                os.remove(filename)
        except OSError:
            pass

        command = SubGen.run_command + " " + graphSample + " " + str(seed)
        os.system(command)
        graph = self.read_graph(filename, label)
        return graph

    def create_synthetic_dataset(self, dataset):
        g_list = {}
        if dataset.dataset_name == 'SD1':
            for i in range(1, dataset.total_graphs + 1):
                if i <= dataset.drift_points[0]:  # First half same label
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "pos")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "neg")
                        # print("First Half Graph Sent", i)
                else:  # Second half from S2
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "pos")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "neg")
                        g_list[i] = self.create_synthetic_graph(SubGen.s2_negative_sample, i, "pos")

        elif dataset.dataset_name == 'SD2':
            for i in range(1, dataset.total_graphs + 1):
                if i <= 1000:
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "pos")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "neg")
                elif i > 1000 and i <= 2000:  # Second half from S2
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "neg")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "pos")
                        # print("Second :", i)
                elif i > 2000 and i <= 3000:
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "pos")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "neg")
                        # print("Third: ", i)
                elif i > 3000 and i <= 4000:
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "neg")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "pos")
                        # print("fourth: ", i)
                elif i > 4000 and i <= 5000:
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "pos")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "neg")
                        # print("Fifth:", i)
                elif i > 5000:
                    if i % 9 == 0:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_negative_sample, i, "neg")
                    else:
                        g_list[i] = self.create_synthetic_graph(SubGen.s1_positive_sample, i, "pos")
        return g_list

    def read_send_gfile(self, fileName):

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

    def save_graph_file(self, message, graphCount, dataset_name):
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
        import collections
        #message =collections.OrderedDict(sorted(message['node']))
        node = []
        for n in message['node'].keys():
            node.append(int(n))

        #print("Message: ", sorted(node))

        fileName = GBAD.graph_folder + "/" + dataset_name + '.g'
        fw = open(fileName, "w")
        fw.write("XP # 1 //" + str(graphCount) + "\n")
        for key in sorted(node[:]):
            #print("Key: ", node)
            fw.write("v " + str(key) + " \"" + message['node'][str(key)] + "\"\n")
        for key in message['edge']:
            k = key.split(' ', 1)
            if message['edge'][key]:
                fw.write("d " + k[0] + " " + k[1] + " \"" + message['edge'][key] + "\"\n")
            else:
                fw.write("d " + k[0] + " " + k[1] + "\n")

        return fileName

    def read_subgraph(self, subgraphFile):
        # This is to read in networkx format
        s = {}
        instance = 0
        with open(subgraphFile) as f:
            lines = f.readlines()
            for l in lines:
                item = l.split(' ')
                if l == "\n":
                    s[
                        sub_graph] = instance  # insert total instance count, new window and old window..  old window 0 this time
                elif item[0] == 'S':
                    sub_graph = nx.DiGraph()
                    instance = int(item[1].strip('\n').strip(' '))
                elif item[0] == 'v':
                    sub_graph.add_node(item[1], label=item[2].strip('\n').strip('\"'))
                elif item[0] == 'u' or item[0] == 'd':
                    sub_graph.add_edge(item[1], item[2], label=item[3].strip('\n').strip('\"'))
        return s

    def generate_discriminative_subgraph(self, graph_file, dataset_name, param_n):
        subgraph_file = GBAD.graph_folder + "/" + "SG_" + dataset_name + '.g'
        output_file = GBAD.graph_folder + "/" + "out_" + dataset_name +'.txt'
        try:
            os.remove(subgraph_file)
            os.remove(output_file)
        except OSError:
            pass

        command = GBAD.gbad_home + "/" + GBAD.run_command + " -nsubs " + str(
            param_n) + " -out " + subgraph_file + " " + graph_file + ">>" + output_file

        # print(command)
        os.system(command)
        return self.read_subgraph(subgraph_file)


    def initialize_dataset(self, d):# dname, tgraph, dpoints, fname, type):
        #try:
            ds = Dataset()
            ds.dataset_name = d[0]
            ds.total_graphs = d[1]
            ds.drift_points = d[2]
            ds.file_name = d[3]
            if d[4] == True:
                ds.is_synthetic = True
                g_list = self.create_synthetic_dataset(ds)
                self.save_synthetic_graph(g_list, ds.file_name)
            else:
                ds.is_synthetic = False

            ds.param = d[5]

            #setting subgraph_list
            g_list = self.read_send_gfile(d[3])

            for g_id in g_list.keys():
                #print("G : ", g_id, g_list[g_id])
                graphFile = self.save_graph_file(g_list[g_id], g_id, d[0])
                #print("Graph File: ", graphFile)
                s = self.generate_discriminative_subgraph(graphFile, d[0], d[5][0])
                ds.subgraph_list[g_id] = s
                #print("G: ", g_id, " \n S: ", s)
                #ds.subgraph_list[g_id] = self.generate_discriminative_subgraph(graphFile, d[0], d[5][0])

            print('----- Dataset [ %s ] Created Successfully----' % (d[0]))
            return ds
        #except Exception as e:
        #    print('----- Couldnot Create Dataset [ %s ] ----' % (d[0]))
        #    return None

    def get_available_dataset(self):
        # initialize all dataset
        available_dataset = []
        for d in DataList.data_list:
            available_dataset.append(self.initialize_dataset(d)) #d[0], d[1], d[2], d[3], d[4]))
        return available_dataset
