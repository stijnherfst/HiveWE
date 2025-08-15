module Triggers;

import std;

std::string Triggers::get_type(const std::string_view function_name, const size_t parameter) const {
	std::string type;

	if (trigger_data.key_exists("TriggerActions", function_name)) {
		type = trigger_data.data("TriggerActions", function_name, 1 + parameter);
	} else if (trigger_data.key_exists("TriggerCalls", function_name)) {
		type = trigger_data.data("TriggerCalls", function_name, 3 + parameter);
	} else if (trigger_data.key_exists("TriggerEvents", function_name)) {
		type = trigger_data.data("TriggerEvents", function_name, 1 + parameter);
	} else if (trigger_data.key_exists("TriggerConditions", function_name)) {
		type = trigger_data.data("TriggerConditions", function_name, 1 + parameter);
	}
	return type;
}

std::string generate_function_name(const std::string& trigger_name) {
	const auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return "Trig_" + trigger_name + "_" + std::to_string(time & 0xFFFFFFFF);
}

std::string Triggers::resolve_parameter(
	const TriggerParameter& parameter,
	const std::string& trigger_name,
	MapScriptWriter& pre_actions,
	const std::string& type,
	ScriptMode mode,
	bool add_call = false
) const {
	switch (parameter.type) {
		case TriggerParameter::Type::invalid:
			std::print("Invalid parameter type\n");
			return "";
		case TriggerParameter::Type::constant: {
			const std::string constant_type = trigger_data.data("TriggerParams", parameter.value, 1);

			if (get_base_type(constant_type, trigger_data) == "string") {
				return string_replaced(trigger_data.data("TriggerParams", parameter.value, 2), "`", "\"");
			}

			if (constant_type == "timedlifebuffcode" // ToDo this seems like a hack?
				|| type == "abilcode" || type == "buffcode" || type == "destructablecode" || type == "itemcode" || type == "ordercode"
				|| type == "techcode" || type == "unitcode" || type == "heroskillcode" || type == "weathereffectcode"
				|| type == "timedlifebuffcode" || type == "doodadcode" || type == "timedlifebuffcode" || type == "terraintype") {
				return "FourCC(" + trigger_data.data("TriggerParams", parameter.value, 2) + ")";
			}

			return trigger_data.data("TriggerParams", parameter.value, 2);
		}
		case TriggerParameter::Type::function:
			if (parameter.has_sub_parameter) {
				return convert_eca_to_script(parameter.sub_parameter, pre_actions, trigger_name, mode, add_call);
			} else {
				return parameter.value + "()";
			}
		case TriggerParameter::Type::variable: {
			std::string output = parameter.value;

			if (!output.starts_with("gg_")) {
				output = "udg_" + output;
			}

			if (parameter.is_array) {
				output += "[" + resolve_parameter(parameter.parameters[0], trigger_name, pre_actions, "integer", mode) + "]";
			}
			return output;
		}
		case TriggerParameter::Type::string:
			const std::string import_type = trigger_data.data("TriggerTypes", type, 5);

			if (!import_type.empty()) {
				return "\"" + string_replaced(parameter.value, "\\", "\\\\") + "\"";
			} else if (get_base_type(type, trigger_data) == "string") {
				return "\"" + parameter.value + "\"";
			} else if (type == "abilcode" // ToDo this seems like a hack?
					   || type == "buffcode" || type == "destructablecode" || type == "itemcode" || type == "ordercode"
					   || type == "techcode" || type == "unitcode" || type == "heroskillcode" || type == "weathereffectcode"
					   || type == "timedlifebuffcode" || type == "doodadcode" || type == "timedlifebuffcode" || type == "terraintype") {
				return "FourCC('" + parameter.value + "')";
			} else {
				return parameter.value;
			}
	}
	std::print("Unable to resolve parameter for trigger: {} and parameter value {}\n", trigger_name, parameter.value);
	return "";
}

std::string Triggers::convert_eca_to_script(
	const ECA& eca,
	MapScriptWriter& pre_actions,
	const std::string& trigger_name,
	ScriptMode mode,
	bool add_call
) const {
	if (!eca.enabled) {
		return "";
	}

	if (eca.name == "IfThenElse") {
		const std::string function_name = generate_function_name(trigger_name);

		pre_actions.function(
			function_name,
			[&] {
				pre_actions.write_ln(
					"return ",
					resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0), mode)
				);
			},
			"takes nothing returns boolean"
		);

		MapScriptWriter writer(mode);

		writer.if_else_statement(
			function_name + "()",
			[&] {
				writer.write_ln(resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1), mode, true));
			},
			[&] {
				writer.write_ln(resolve_parameter(eca.parameters[2], trigger_name, pre_actions, get_type(eca.name, 2), mode, true));
			}
		);

		return writer.script;
	}

	if (eca.name == "IfThenElseMultiple") {
		const std::string function_name = generate_function_name(trigger_name);

		std::vector<std::string> conditions;
		for (const auto& i : eca.ecas) {
			if (i.type != ECA::Type::condition) {
				continue;
			}
			conditions.push_back(convert_eca_to_script(i, pre_actions, trigger_name, mode, false));
		}

		pre_actions.function(
			function_name,
			[&] {
				for (const auto& i : conditions) {
					pre_actions.if_statement(std::format("not ({})", i), [&] {
						pre_actions.write_ln("return false");
					});
				}

				pre_actions.write_ln("return true");
			},
			"takes nothing returns boolean"
		);

		MapScriptWriter writer(mode);

		writer.if_else_statement(
			function_name + "()",
			[&] {
				for (const auto& i : eca.ecas) {
					if (i.type != ECA::Type::action) {
						continue;
					}

					if (i.group == 1) {
						writer.write_ln(convert_eca_to_script(i, pre_actions, trigger_name, mode, true));
					}
				}
			},
			[&] {
				for (const auto& i : eca.ecas) {
					if (i.type != ECA::Type::action) {
						continue;
					}

					// TODO, I suspect group 0 is the if, group 1 is the then and group 2 is the else
					if (i.group != 1) {
						writer.write_ln(convert_eca_to_script(i, pre_actions, trigger_name, mode, true));
					}
				}
			}
		);

		return writer.script;
	}

	if (eca.name.starts_with("ForLoopA") || eca.name.starts_with("ForLoopB")) {
		const std::string loop_index = eca.name.starts_with("ForLoopA") ? "bj_forLoopAIndex" : "bj_forLoopBIndex";
		const std::string loop_index_end = eca.name.starts_with("ForLoopA") ? "bj_forLoopAIndexEnd" : "bj_forLoopBIndexEnd";

		const auto start_at = resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0), mode);
		const auto exit_when = resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1), mode);

		MapScriptWriter writer(mode);

		writer.set_variable(loop_index, start_at);
		// Have to set this as somebody might have used the variable inside
		writer.set_variable(loop_index_end, exit_when);

		writer.while_statement(std::format("{} <= {}", loop_index, loop_index_end), [&] {
			if (eca.name.ends_with("Multiple")) {
				for (const auto& i : eca.ecas) {
					writer.write_ln(convert_eca_to_script(i, pre_actions, trigger_name, mode, true));
				}
			} else {
				writer.write_ln(resolve_parameter(eca.parameters[2], trigger_name, pre_actions, get_type(eca.name, 2), mode, true));
			}

			writer.set_variable(loop_index, loop_index + " + 1");
		});

		return writer.script;
	}

	if (eca.name == "ForLoopVarMultiple" || eca.name == "ForLoopVar") {
		const auto variable = resolve_parameter(eca.parameters[0], trigger_name, pre_actions, "integer", mode);
		const auto start_at = resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1), mode);
		const auto exit_when = resolve_parameter(eca.parameters[2], trigger_name, pre_actions, get_type(eca.name, 2), mode);

		MapScriptWriter writer(mode);
		writer.set_variable(variable, start_at);
		writer.while_statement(std::format("{} <= {}", variable, exit_when), [&] {
			if (eca.name == "ForLoopVarMultiple") {
				for (const auto& i : eca.ecas) {
					writer.write_ln(convert_eca_to_script(i, pre_actions, trigger_name, mode, true));
				}
			} else {
				writer.write_ln(resolve_parameter(eca.parameters[3], trigger_name, pre_actions, get_type(eca.name, 3), mode, true));
			}

			writer.set_variable(variable, variable + " + 1");
		});

		return writer.script;
	}

	if (eca.name == "AndMultiple" || eca.name == "OrMultiple") {
		const std::string function_name = generate_function_name(trigger_name);

		std::vector<std::string> conditions;
		for (const auto& i : eca.ecas) {
			conditions.push_back(convert_eca_to_script(i, pre_actions, trigger_name, mode, false));
		}

		pre_actions.function(
			function_name,
			[&] {
				if (eca.name == "AndMultiple") {
					for (const auto& i : conditions) {
						pre_actions.if_statement(std::format("not({})", i), [&] {
							pre_actions.write_ln("return false");
						});
					}

					pre_actions.write_ln("return true");
				} else {
					for (const auto& i : conditions) {
						pre_actions.if_statement(i, [&] {
							pre_actions.write_ln("return true");
						});
					}

					pre_actions.write_ln("return false");
				}
			},
			"takes nothing returns boolean"
		);

		return function_name + "()";
	}

	if (eca.name == "SetVariable") {
		const std::string& type = std::ranges::find_if(variables, [&](const TriggerVariable& var) {
									  return var.name == eca.parameters[0].value;
								  })->type;
		const std::string first = resolve_parameter(eca.parameters[0], trigger_name, pre_actions, "", mode);
		const std::string second = resolve_parameter(eca.parameters[1], trigger_name, pre_actions, type, mode);

		MapScriptWriter writer(mode);
		writer.set_variable(first, second);
		return writer.script;
	}

	if (eca.name == "CommentString") {
		// TODO comments in lua are with --
		return "//" + resolve_parameter(eca.parameters[0], trigger_name, pre_actions, "", mode);
	}

	if (eca.name == "CustomScriptCode") {
		return resolve_parameter(eca.parameters[0], trigger_name, pre_actions, "", mode);
	}

	std::vector<std::string> resolved_parameters;
	for (size_t i = 0; i < eca.parameters.size(); ++i) {
		resolved_parameters.push_back(resolve_parameter(eca.parameters[i], trigger_name, pre_actions, get_type(eca.name, i), mode));
	}

	if (eca.name == "ForForceMultiple" || eca.name == "ForGroupMultiple" || eca.name == "EnumDestructablesInRectAllMultiple"
		|| eca.name == "EnumDestructablesInCircleBJMultiple") {
		const std::string script_name = trigger_data.data("TriggerActions", "_" + eca.name + "_ScriptName");

		const std::string function_name = generate_function_name(trigger_name);

		std::vector<std::string> ecas;
		for (const auto& i : eca.ecas) {
			ecas.push_back(convert_eca_to_script(i, pre_actions, trigger_name, mode, true));
		}

		pre_actions.function(function_name, [&] {
			for (const auto& i : ecas) {
				pre_actions.write_ln(i);
			}
		});

		MapScriptWriter writer(mode);

		if (eca.name == "EnumDestructablesInCircleBJMultiple") {
			writer.call(script_name, resolved_parameters[0], resolved_parameters[1], "function " + function_name);
		} else {
			writer.call(script_name, resolved_parameters[0], "function " + function_name);
		}

		return writer.script;
	}

	if (eca.name == "WaitForCondition") {
		MapScriptWriter writer(mode);
		writer.while_statement(std::format("not ({})", resolved_parameters[0]), [&] {
			writer.call("TriggerSleepAction", std::format("RMaxBJ(bj_WAIT_FOR_COND_MIN_INTERVAL, {})", resolved_parameters[1]));
		});

		return writer.script;
	}

	if (eca.name.starts_with("OperatorCompare") || eca.name == "OperatorInt" || eca.name == "OperatorReal") {
		auto result_operator = resolved_parameters[1];
		if (result_operator == "!=" && mode == ScriptMode::lua) {
			result_operator = "~=";
		}

		return std::format("{} {} {}", resolved_parameters[0], result_operator, resolved_parameters[2]);
	}

	if (eca.name == "OperatorString") {
		if (mode == ScriptMode::jass) {
			return std::format("({} + {})\n", resolved_parameters[0], resolved_parameters[1]);
		} else {
			return std::format("({} .. {})\n", resolved_parameters[0], resolved_parameters[1]);
		}
	}

	if (eca.name == "AddTriggerEvent") {
		return resolved_parameters[1].insert(resolved_parameters[1].find_first_of('(') + 1, resolved_parameters[0] + ", ");
	}

	std::string output;
	for (size_t k = 0; k < eca.parameters.size(); k++) {
		const std::string type = get_type(eca.name, k);
		if (type == "boolexpr") {
			const std::string function_name = generate_function_name(trigger_name);
			pre_actions.function(
				function_name,
				[&] {
					pre_actions.write_ln("return ", resolved_parameters[k]);
				},
				"takes nothing returns boolean"
			);
			output += function_name;
		}
		if (type == "boolcall") {
			const std::string function_name = generate_function_name(trigger_name);
			pre_actions.function(
				function_name,
				[&] {
					pre_actions.write_ln("return ", resolved_parameters[k]);
				},
				"takes nothing returns boolean"
			);
			output += function_name + "()";
		} else if (type == "code") {
			const std::string function_name = generate_function_name(trigger_name);
			pre_actions.function(function_name, [&] {
				pre_actions.write_ln(resolved_parameters[k]);
			});
			output += function_name;
		} else {
			output += resolved_parameters[k];
		}

		if (k < eca.parameters.size() - 1) {
			output += ", ";
		}
	}

	const std::string script_name = trigger_data.data("TriggerActions", "_" + eca.name + "_ScriptName");
	return (add_call ? "call " : "") + (script_name.empty() ? eca.name : script_name) + "(" + output + ")";
}

std::string Triggers::convert_gui_to_jass(const Trigger& trigger, std::vector<std::string>& map_initializations, ScriptMode mode) const {
	std::string trigger_name = trigger.name;
	trim(trigger_name);
	std::ranges::replace(trigger_name, ' ', '_');

	const std::string trigger_variable_name = "gg_trg_" + trigger_name;
	const std::string trigger_action_name = "Trig_" + trigger_name + "_Actions";
	const std::string trigger_conditions_name = "Trig_" + trigger_name + "_Conditions";

	MapScriptWriter events(mode);
	MapScriptWriter conditions(mode);

	MapScriptWriter actions(mode);
	MapScriptWriter pre_actions(mode);

	for (const auto& i : trigger.ecas) {
		if (!i.enabled) {
			continue;
		}

		switch (i.type) {
			case ECA::Type::event: {
				if (i.name == "MapInitializationEvent") {
					map_initializations.push_back(trigger_variable_name);
					continue;
				}

				std::string arguments;

				for (size_t k = 0; k < i.parameters.size(); k++) {
					const auto& p = i.parameters[k];

					if (get_type(i.name, k) == "VarAsString_Real") {
						arguments += "\"" + resolve_parameter(p, trigger_name, pre_actions, get_type(i.name, k), mode) + "\"";
					} else {
						arguments += resolve_parameter(p, trigger_name, pre_actions, get_type(i.name, k), mode);
					}

					if (k < i.parameters.size() - 1) {
						arguments += ", ";
					}
				}

				events.call(i.name, trigger_variable_name, arguments);

				break;
			}
			case ECA::Type::condition:
				conditions.if_statement(std::format("not ({})", convert_eca_to_script(i, pre_actions, trigger_name, mode, false)), [&] {
					conditions.write_ln("return false");
				});
				break;
			case ECA::Type::action:
				actions.write_ln(convert_eca_to_script(i, pre_actions, trigger_name, mode, false));
				break;
		}
	}

	MapScriptWriter final_trigger(mode);

	final_trigger.merge(pre_actions);

	if (!conditions.is_empty()) {
		final_trigger.function(
			trigger_conditions_name,
			[&] {
				final_trigger.merge(conditions);
				final_trigger.write_ln("return true");
			},
			"takes nothing returns boolean"
		);
	}

	final_trigger.function(trigger_action_name, [&] {
		final_trigger.merge(actions);
	});

	final_trigger.function("InitTrig_" + trigger_name, [&] {
		final_trigger.set_variable(trigger_variable_name, "CreateTrigger()");
		final_trigger.merge(events);

		if (!conditions.is_empty()) {
			if (mode == ScriptMode::jass) {
				final_trigger.call("TriggerAddCondition", trigger_variable_name, "Condition( function " + trigger_conditions_name + ")");
			} else {
				final_trigger.call("TriggerAddCondition", trigger_variable_name, trigger_conditions_name);
			}
		}
		if (!trigger.initially_on) {
			final_trigger.call("DisableTrigger", trigger_variable_name);
		}

		if (mode == ScriptMode::jass) {
			final_trigger.call("TriggerAddAction", trigger_variable_name, "function " + trigger_action_name);
		} else {
			final_trigger.call("TriggerAddAction", trigger_variable_name, trigger_action_name);
		}
	});

	return final_trigger.script;
}