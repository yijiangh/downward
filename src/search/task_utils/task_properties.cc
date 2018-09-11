#include "task_properties.h"

#include "../utils/memory.h"
#include "../utils/system.h"

#include <algorithm>
#include <iostream>
#include <limits>

using namespace std;
using utils::ExitCode;


namespace task_properties {
static unordered_map<const AbstractTask *,
                     unique_ptr<int_packer::IntPacker>> state_packer_cache;

bool is_unit_cost(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        if (op.get_cost() != 1)
            return false;
    }
    return true;
}

bool has_axioms(TaskProxy task) {
    return !task.get_axioms().empty();
}

void verify_no_axioms(TaskProxy task) {
    if (has_axioms(task)) {
        cerr << "This configuration does not support axioms!"
             << endl << "Terminating." << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
}


static int get_first_conditional_effects_op_id(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            if (!effect.get_conditions().empty())
                return op.get_id();
        }
    }
    return -1;
}

bool has_conditional_effects(TaskProxy task) {
    return get_first_conditional_effects_op_id(task) != -1;
}

void verify_no_conditional_effects(TaskProxy task) {
    int op_id = get_first_conditional_effects_op_id(task);
    if (op_id != -1) {
        OperatorProxy op = task.get_operators()[op_id];
        cerr << "This configuration does not support conditional effects "
             << "(operator " << op.get_name() << ")!" << endl
             << "Terminating." << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
}

vector<int> get_operator_costs(const TaskProxy &task_proxy) {
    vector<int> costs;
    OperatorsProxy operators = task_proxy.get_operators();
    costs.reserve(operators.size());
    for (OperatorProxy op : operators)
        costs.push_back(op.get_cost());
    return costs;
}

double get_average_operator_cost(TaskProxy task_proxy) {
    double average_operator_cost = 0;
    for (OperatorProxy op : task_proxy.get_operators()) {
        average_operator_cost += op.get_cost();
    }
    average_operator_cost /= task_proxy.get_operators().size();
    return average_operator_cost;
}

int get_min_operator_cost(TaskProxy task_proxy) {
    int min_cost = numeric_limits<int>::max();
    for (OperatorProxy op : task_proxy.get_operators()) {
        min_cost = min(min_cost, op.get_cost());
    }
    return min_cost;
}

const int_packer::IntPacker &get_state_packer(const AbstractTask *task) {
    if (state_packer_cache.count(task) == 0) {
        TaskProxy task_proxy(*task);
        VariablesProxy variables = task_proxy.get_variables();
        vector<int> variable_ranges;
        variable_ranges.reserve(variables.size());
        for (VariableProxy var : variables) {
            variable_ranges.push_back(var.get_domain_size());
        }
        state_packer_cache.insert(
            make_pair(task, utils::make_unique_ptr<int_packer::IntPacker>(variable_ranges)));
    }
    return *state_packer_cache[task];
}
}
