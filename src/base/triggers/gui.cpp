module Triggers;

import std;

std::string Triggers::get_type(const std::string& function_name, const int parameter) const {
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
	auto time = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return "Trig_" + trigger_name + "_" + std::to_string(time & 0xFFFFFFFF);
}

std::string Triggers::resolve_parameter(
	const TriggerParameter& parameter,
	const std::string& trigger_name,
	std::string& pre_actions,
	const std::string& type,
	const bool add_call = false
) const {
	if (parameter.has_sub_parameter) {
		return testt(trigger_name, parameter.sub_parameter.name, parameter.sub_parameter.parameters, pre_actions, add_call);
	} else {
		switch (parameter.type) {
			case TriggerParameter::Type::invalid:
				std::print("Invalid parameter type\n");
				return "";
			case TriggerParameter::Type::preset: {
				const std::string preset_type = trigger_data.data("TriggerParams", parameter.value, 1);

				if (get_base_type(preset_type, trigger_data) == "string") {
					return string_replaced(trigger_data.data("TriggerParams", parameter.value, 2), "`", "\"");
				}

				if (preset_type == "timedlifebuffcode" // ToDo this seems like a hack?
					|| type == "abilcode" || type == "buffcode" || type == "destructablecode" || type == "itemcode" || type == "ordercode"
					|| type == "techcode" || type == "unitcode" || type == "heroskillcode" || type == "weathereffectcode"
					|| type == "timedlifebuffcode" || type == "doodadcode" || type == "timedlifebuffcode" || type == "terraintype") {
					return "FourCC(" + trigger_data.data("TriggerParams", parameter.value, 2) + ")";
				}

				return trigger_data.data("TriggerParams", parameter.value, 2);
			}
			case TriggerParameter::Type::function:
				return parameter.value + "()";
			case TriggerParameter::Type::variable: {
				std::string output = parameter.value;

				if (!output.starts_with("gg_")) {
					output = "udg_" + output;
				}

				if (parameter.is_array) {
					output += "[" + resolve_parameter(parameter.parameters[0], trigger_name, pre_actions, "integer") + "]";
				}
				return output;
			}
			case TriggerParameter::Type::string:
				std::string import_type = trigger_data.data("TriggerTypes", type, 5);

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
	}
	std::print("Unable to resolve parameter for trigger: {} and parameter value {}\n", trigger_name, parameter.value);
	return "";
}

std::string Triggers::convert_eca_to_jass(const ECA& eca, std::string& pre_actions, const std::string& trigger_name, bool nested) const {
	std::string output;

	if (!eca.enabled) {
		return "";
	}

	if (eca.name == "WaitForCondition") {
		output += "while (true) do\n";
		output += std::format(
			"if (({})) then break end\n",
			resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0))
		);
		output += "TriggerSleepAction(RMaxBJ(bj_WAIT_FOR_COND_MIN_INTERVAL, "
			+ resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + "))\n";
		output += "end\n";

		return output;
	}

	if (eca.name == "ForLoopAMultiple" || eca.name == "ForLoopBMultiple") {
		const std::string loop_index = eca.name == "ForLoopAMultiple" ? "bj_forLoopAIndex" : "bj_forLoopBIndex";
		const std::string loop_index_end = eca.name == "ForLoopAMultiple" ? "bj_forLoopAIndexEnd" : "bj_forLoopBIndexEnd";

		output += loop_index + "=" + resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0)) + "\n";
		output += loop_index_end + "=" + resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + "\n";
		output += "while (true) do\n";
		output += std::format("if (({} > {})) then break end\n", loop_index, loop_index_end);
		for (const auto& i : eca.ecas) {
			output += "" + convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
		}
		output += loop_index + " = " + loop_index + " + 1\n";
		output += "end\n";

		return output;
	}

	if (eca.name == "ForLoopVarMultiple") {
		std::string variable = resolve_parameter(eca.parameters[0], trigger_name, pre_actions, "integer");

		output += variable + " = " + resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + "\n";
		output += "while (true) do\n";
		output += std::format(
			"if (({} > {})) then break end\n",
			variable,
			resolve_parameter(eca.parameters[2], trigger_name, pre_actions, get_type(eca.name, 2))
		);
		for (const auto& i : eca.ecas) {
			output += convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
		}
		output += variable + " = " + variable + " + 1\n";
		output += "end\n";

		return output;
	}

	if (eca.name == "IfThenElseMultiple") {
		std::string iftext;
		std::string thentext;
		std::string elsetext;

		std::string function_name = generate_function_name(trigger_name);
		iftext += "function " + function_name + "()\n"; // returns boolean

		for (const auto& i : eca.ecas) {
			if (i.type == ECA::Type::condition) {
				iftext += "if (not (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ")) then\n";
				iftext += "return false\n";
				iftext += "end\n";
			} else if (i.type == ECA::Type::action) {
				if (i.group == 1) {
					thentext += convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
				} else {
					elsetext += convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
				}
			}
		}
		iftext += "return true\n";
		iftext += "end\n";
		pre_actions += iftext;

		return "if (" + function_name + "()) then\n" + thentext + "else\n" + elsetext + "end";
	}

	if (eca.name == "ForForceMultiple" || eca.name == "ForGroupMultiple" || eca.name == "EnumDestructablesInRectAllMultiple"
		|| eca.name == "EnumDestructablesInCircleBJMultiple") {
		std::string script_name = trigger_data.data("TriggerActions", "_" + eca.name + "_ScriptName");

		const std::string function_name = generate_function_name(trigger_name);

		if (eca.name == "EnumDestructablesInCircleBJMultiple") {
			output += script_name + "(" + resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0)) + ", "
				+ resolve_parameter(eca.parameters[1], trigger_name, pre_actions, get_type(eca.name, 1)) + ", function " + function_name
				+ ")\n";
		} else {
			output += script_name + "(" + resolve_parameter(eca.parameters[0], trigger_name, pre_actions, get_type(eca.name, 0))
				+ ", function " + function_name + ")\n";
		}

		std::string toto;
		for (const auto& i : eca.ecas) {
			toto += "" + convert_eca_to_jass(i, pre_actions, trigger_name, false) + "\n";
		}
		pre_actions += "function " + function_name + "()\n"; // returns nothing
		pre_actions += toto;
		pre_actions += "\nend\n";

		return output;
	}

	if (eca.name == "AndMultiple") {
		const std::string function_name = generate_function_name(trigger_name);

		std::string iftext = "function " + function_name + "()\n"; // returns boolean
		for (const auto& i : eca.ecas) {
			iftext += "if (not (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ")) then\n";
			iftext += "return false\n";
			iftext += "end\n";
		}
		iftext += "return true\n";
		iftext += "end\n";
		pre_actions += iftext;

		return function_name + "()";
	}

	if (eca.name == "OrMultiple") {
		const std::string function_name = generate_function_name(trigger_name);

		std::string iftext = "function " + function_name + "()\n"; // returns boolean
		for (const auto& i : eca.ecas) {
			iftext += "if (" + convert_eca_to_jass(i, pre_actions, trigger_name, true) + ") then\n";
			iftext += "return true\n";
			iftext += "end\n";
		}
		iftext += "return false\n";
		iftext += "end\n";
		pre_actions += iftext;

		return function_name + "()";
	}

	return testt(trigger_name, eca.name, eca.parameters, pre_actions, !nested);
}

std::string Triggers::testt(
	const std::string& trigger_name,
	const std::string& parent_name,
	const std::vector<TriggerParameter>& parameters,
	std::string& pre_actions,
	bool add_call
) const {
	std::string output;

	std::string script_name = trigger_data.data("TriggerActions", "_" + parent_name + "_ScriptName");

	if (parent_name == "SetVariable") {
		const std::string& type =
			(*std::ranges::find_if(variables, [parameters](const TriggerVariable& var) { return var.name == parameters[0].value; })).type;
		const std::string first = resolve_parameter(parameters[0], trigger_name, pre_actions, "");
		const std::string second = resolve_parameter(parameters[1], trigger_name, pre_actions, type);

		return first + " = " + second;
	}

	if (parent_name == "CommentString") {
		return "//" + resolve_parameter(parameters[0], trigger_name, pre_actions, "");
	}

	if (parent_name == "CustomScriptCode") {
		return resolve_parameter(parameters[0], trigger_name, pre_actions, "");
	}

	if (parent_name.substr(0, 15) == "OperatorCompare") {
		output += resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));

		auto result_operator = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
		if (result_operator == "!=") {
			result_operator = "~=";
		}

		output += " " + result_operator + " ";
		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2));
		return output;
	}

	if (parent_name == "OperatorString") {
		output += "(" + resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		output += " .. ";
		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + ")";
		return output;
	}

	if (parent_name == "ForLoopVar") {
		std::string variable = resolve_parameter(parameters[0], trigger_name, pre_actions, "integer");

		output += variable + " = ";
		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1)) + "\n";

		output += "while (true) do\n";
		output += std::format(
			"if (({} > {})) then break end\n",
			variable,
			resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2))
		);
		output += resolve_parameter(parameters[3], trigger_name, pre_actions, get_type(parent_name, 3), true) + "\n";
		output += variable + " = " + variable + " + 1\n";
		output += "end\n";

		return output;
	}

	if (parent_name == "IfThenElse") {
		std::string thentext;
		std::string elsetext;

		std::string function_name = generate_function_name(trigger_name);
		std::string tttt = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));

		output += "if (" + function_name + "())\n";
		output += resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1), true) + "\n";
		output += "else\n";
		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2), true) + "\n";
		output += "end";

		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return " + tttt + "\n";
		pre_actions += "end\n";
		return output;
	}

	if (parent_name == "ForForce" || parent_name == "ForGroup") {
		std::string function_name = generate_function_name(trigger_name);

		std::string tttt = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));

		output += parent_name + "(";
		output += resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		output += ", " + function_name;
		output += ")";

		pre_actions += "function " + function_name + "()\n"; // returns nothing
		pre_actions += tttt + "\n";
		pre_actions += "end\n\n";
		return /*(add_call ? "call " : "") +*/ output;
	}

	if (parent_name == "GetBooleanAnd") {
		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));

		std::string function_name = generate_function_name(trigger_name);
		output += "GetBooleanAnd(" + function_name + "(), ";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + first_parameter + ")\n";
		pre_actions += "end\n\n";

		function_name = generate_function_name(trigger_name);
		output += function_name + "())";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + second_parameter + ")\n";
		pre_actions += "end\n\n";

		return /*(add_call ? "call " : "") +*/ output;
	}

	if (parent_name == "GetBooleanOr") {
		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));

		std::string function_name = generate_function_name(trigger_name);
		output += "GetBooleanOr(" + function_name + "(), ";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + first_parameter + ")\n";
		pre_actions += "end\n\n";

		function_name = generate_function_name(trigger_name);
		output += function_name + "())";
		pre_actions += "function " + function_name + "()\n"; // returns boolean
		pre_actions += "return ( " + second_parameter + ")\n";
		pre_actions += "end\n\n";

		return /*(add_call ? "call " : "") +*/ output;
	}

	if (parent_name == "OperatorInt" || parent_name == "OperatorReal") {
		output += "(" + resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));

		auto result_operator = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
		if (result_operator == "!=") {
			result_operator = "~=";
		}

		output += " " + result_operator + " ";
		output += resolve_parameter(parameters[2], trigger_name, pre_actions, get_type(parent_name, 2)) + ")";
		return output;
	}

	if (parent_name == "AddTriggerEvent") {
		std::string first_parameter = resolve_parameter(parameters[0], trigger_name, pre_actions, get_type(parent_name, 0));
		std::string second_parameter = resolve_parameter(parameters[1], trigger_name, pre_actions, get_type(parent_name, 1));
		output += second_parameter.insert(second_parameter.find_first_of('(') + 1, first_parameter + ", ");
		return /*(add_call ? "call " : "") + */ output;
	}

	for (size_t k = 0; k < parameters.size(); k++) {
		const auto& i = parameters[k];

		const std::string type = get_type(parent_name, k);

		if (type == "boolexpr") {
			const std::string function_name = generate_function_name(trigger_name);

			std::string tttt = resolve_parameter(parameters[k], trigger_name, pre_actions, type);

			pre_actions += "function " + function_name + "()\n"; // returns boolean
			pre_actions += "return " + tttt + "\n";
			pre_actions += "end\n\n";

			output += function_name;
		} else if (type == "code") {
			const std::string function_name = generate_function_name(trigger_name);

			std::string tttt = resolve_parameter(parameters[k], trigger_name, pre_actions, type);

			pre_actions += "function " + function_name + "()\n"; // returns nothing
			pre_actions += tttt + "\n";
			pre_actions += "end\n\n";

			output += function_name;
		} else {
			output += resolve_parameter(i, trigger_name, pre_actions, type);
		}

		if (k < parameters.size() - 1) {
			output += ", ";
		}
	}

	return /*(add_call ? "call " : "") + */ (script_name.empty() ? parent_name : script_name) + "(" + output + ")";
}

std::string Triggers::convert_gui_to_jass(const Trigger& trigger, std::vector<std::string>& map_initializations) const {
	std::string trigger_name = trigger.name;
	trim(trigger_name);
	std::ranges::replace(trigger_name, ' ', '_');

	const std::string trigger_variable_name = "gg_trg_" + trigger_name;
	const std::string trigger_action_name = "Trig_" + trigger_name + "_Actions";
	const std::string trigger_conditions_name = "Trig_" + trigger_name + "_Conditions";

	MapScriptWriter events;
	MapScriptWriter conditions;

	std::string pre_actions;
	MapScriptWriter actions;

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
						arguments += "\"" + resolve_parameter(p, trigger_name, pre_actions, get_type(i.name, k)) + "\"";
					} else {
						arguments += resolve_parameter(p, trigger_name, pre_actions, get_type(i.name, k));
					}

					if (k < i.parameters.size() - 1) {
						arguments += ", ";
					}
				}

				events.call(i.name, trigger_variable_name, arguments);

				break;
			}
			case ECA::Type::condition:
				conditions.if_statement(std::format("not ({})", convert_eca_to_jass(i, pre_actions, trigger_name, true)), [&] {
					conditions.write_ln("return false");
				});
				break;
			case ECA::Type::action:
				actions.write_ln(convert_eca_to_jass(i, pre_actions, trigger_name, false));
				break;
		}
	}

	MapScriptWriter final_trigger;

	final_trigger.script += pre_actions;

	if (!conditions.is_empty()) {
		final_trigger.function(trigger_conditions_name, [&] {
			final_trigger.merge(conditions);
			final_trigger.write_ln("return true");
		});
	}

	final_trigger.function(trigger_action_name, [&] { final_trigger.merge(actions); });

	final_trigger.function("InitTrig_" + trigger_name, [&] {
		final_trigger.set_variable(trigger_variable_name, "CreateTrigger()");
		final_trigger.merge(events);

		if (!conditions.is_empty()) {
			final_trigger.call("TriggerAddCondition", trigger_variable_name, trigger_conditions_name);
		}
		if (!trigger.initially_on) {
			final_trigger.call("DisableTrigger", trigger_variable_name);
		}
		final_trigger.call("TriggerAddAction", trigger_variable_name, trigger_action_name);
	});

	return final_trigger.script;

	// return pre_actions + conditions + actions + events;
}