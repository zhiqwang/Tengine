/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2021, OPEN AI LAB
 * Author: qtang@openailab.com
 */

#include "scale_param.h"

#include "graph/tensor.h"
#include "graph/node.h"
#include "graph/graph.h"
#include "utility/sys_port.h"
#include "utility/float.h"
#include "utility/log.h"
#include "device/cpu/cpu_node.h"
#include "device/cpu/cpu_graph.h"
#include "device/cpu/cpu_module.h"


int ref_scale_fp32(struct tensor* input_tensor, struct tensor* gamma_tensor, struct tensor* beta_tensor,
                   struct tensor* output_tensor, struct scale_param* param, int num_thread)
{
    int n = input_tensor->dims[0];
    int channel = input_tensor->dims[1];
    int h = input_tensor->dims[2];
    int w = input_tensor->dims[3];

    int nstep = channel * h * w;
    int cstep = h * w;

    float* input_data = (float*)input_tensor->data;
    float* gamma_data = (float*)gamma_tensor->data;
    float* output_data = (float*)output_tensor->data;

    if (beta_tensor == NULL)
    {
        for (int b = 0; b < n; b++)
        {
            for (int c = 0; c < channel; c++)
            {
                int offset = b * nstep + c * cstep;
                for (int i = 0; i < cstep; i++)
                {
                    output_data[offset + i] = input_data[offset + i] * gamma_data[c];
                }
            }
        }
    }
    else
    {
        float* beta_data = (float*)beta_tensor->data;

        for (int b = 0; b < n; b++)
        {
            for (int c = 0; c < channel; c++)
            {
                int offset = b * nstep + c * cstep;
                for (int i = 0; i < cstep; i++)
                {
                    output_data[offset + i] = input_data[offset + i] * gamma_data[c] + beta_data[c];
                }
            }
        }
    }

    return 0;
}

static int init_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int release_node(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    return 0;
}

static int run(struct node_ops* node_ops, struct exec_node* exec_node, struct exec_graph* exec_graph)
{
    struct node* ir_node = exec_node->ir_node;
    struct graph* ir_graph = ir_node->graph;
    struct tensor* input_tensor;
    struct tensor* gamma_tensor;
    struct tensor* beta_tensor = NULL;
    struct tensor* output_tensor;

    input_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[0]);
    gamma_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[1]);

    if (ir_node->input_num == 3)
        beta_tensor = get_ir_graph_tensor(ir_graph, ir_node->input_tensors[2]);

    output_tensor = get_ir_graph_tensor(ir_graph, ir_node->output_tensors[0]);
    struct scale_param* scale_param = ( struct scale_param* )ir_node->op.param_mem;

    ref_scale_fp32(input_tensor, gamma_tensor, beta_tensor, output_tensor, scale_param, exec_graph->num_thread);

    return 0;
}

static int score(struct node_ops* node_ops, struct exec_graph* exec_graph, struct node* exec_node)
{
    return OPS_SCORE_CANDO;
}

static struct node_ops hcl_node_ops = {.prerun = NULL,
                                       .run = run,
                                       .reshape = NULL,
                                       .postrun = NULL,
                                       .init_node = init_node,
                                       .release_node = release_node,
                                       .score = score};

int register_scale_ref_op()
{
    return register_builtin_node_ops(OP_SCALE, &hcl_node_ops);
}

int unregister_scale_ref_op()
{
    return unregister_builtin_node_ops(OP_SCALE, &hcl_node_ops);
}
