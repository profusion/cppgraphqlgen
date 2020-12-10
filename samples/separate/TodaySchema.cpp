// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include "graphqlservice/Introspection.h"
#include "graphqlservice/GraphQLValidation.h"

#include <algorithm>
#include <array>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::literals;

namespace graphql {
namespace service {

static const std::array<std::string_view, 4> s_namesTaskState = {
	"New",
	"Started",
	"Complete",
	"Unassigned"
};

template <>
today::TaskState ModifiedArgument<today::TaskState>::convert(const response::Value& value)
{
	if (!value.maybe_enum())
	{
		throw service::schema_exception { { "not a valid TaskState value" } };
	}

	auto itr = std::find(s_namesTaskState.cbegin(), s_namesTaskState.cend(), value.get<response::StringType>());

	if (itr == s_namesTaskState.cend())
	{
		throw service::schema_exception { { "not a valid TaskState value" } };
	}

	return static_cast<today::TaskState>(itr - s_namesTaskState.cbegin());
}

template <>
std::future<response::Value> ModifiedResult<today::TaskState>::convert(service::FieldResult<today::TaskState>&& result, ResolverParams&& params)
{
	return resolve(std::move(result), std::move(params),
		[](today::TaskState&& value, const ResolverParams&)
		{
			response::Value result(response::Type::EnumValue);

			result.set<response::StringType>(std::string(s_namesTaskState[static_cast<size_t>(value)]));

			return result;
		});
}

template <>
today::CompleteTaskInput ModifiedArgument<today::CompleteTaskInput>::convert(const response::Value& value)
{
	const auto defaultValue = []()
	{
		response::Value values(response::Type::Map);
		response::Value entry;

		entry = response::Value(true);
		values.emplace_back("isComplete", std::move(entry));

		return values;
	}();

	auto valueId = service::ModifiedArgument<response::IdType>::require("id", value);
	auto pairIsComplete = service::ModifiedArgument<response::BooleanType>::find<service::TypeModifier::Nullable>("isComplete", value);
	auto valueIsComplete = (pairIsComplete.second
		? std::move(pairIsComplete.first)
		: service::ModifiedArgument<response::BooleanType>::require<service::TypeModifier::Nullable>("isComplete", defaultValue));
	auto valueClientMutationId = service::ModifiedArgument<response::StringType>::require<service::TypeModifier::Nullable>("clientMutationId", value);

	return {
		std::move(valueId),
		std::move(valueIsComplete),
		std::move(valueClientMutationId)
	};
}

} /* namespace service */

namespace today {

class ValidationContext : public service::ValidationContext
{
public:
	ValidationContext()
	{
		auto typeBoolean = makeNamedValidateType(service::ScalarType { "Boolean" });
		auto typeFloat = makeNamedValidateType(service::ScalarType { "Float" });
		auto typeID = makeNamedValidateType(service::ScalarType { "ID" });
		auto typeInt = makeNamedValidateType(service::ScalarType { "Int" });
		auto typeString = makeNamedValidateType(service::ScalarType { "String" });

		auto typeItemCursor = makeNamedValidateType(service::ScalarType { "ItemCursor" });
		auto typeDateTime = makeNamedValidateType(service::ScalarType { "DateTime" });

		auto typeTaskState = makeNamedValidateType(service::EnumType { "TaskState", {
				"New",
				"Started",
				"Complete",
				"Unassigned"
			} });

		auto typeCompleteTaskInput = makeNamedValidateType(service::InputObjectType { "CompleteTaskInput" });

		auto typeUnionType = makeNamedValidateType(service::UnionType { "UnionType" });

		auto typeNode = makeNamedValidateType(service::InterfaceType { "Node" });

		auto typeQuery = makeNamedValidateType(service::ObjectType { "Query" });
		auto typePageInfo = makeNamedValidateType(service::ObjectType { "PageInfo" });
		auto typeAppointmentEdge = makeNamedValidateType(service::ObjectType { "AppointmentEdge" });
		auto typeAppointmentConnection = makeNamedValidateType(service::ObjectType { "AppointmentConnection" });
		auto typeTaskEdge = makeNamedValidateType(service::ObjectType { "TaskEdge" });
		auto typeTaskConnection = makeNamedValidateType(service::ObjectType { "TaskConnection" });
		auto typeFolderEdge = makeNamedValidateType(service::ObjectType { "FolderEdge" });
		auto typeFolderConnection = makeNamedValidateType(service::ObjectType { "FolderConnection" });
		auto typeCompleteTaskPayload = makeNamedValidateType(service::ObjectType { "CompleteTaskPayload" });
		auto typeMutation = makeNamedValidateType(service::ObjectType { "Mutation" });
		auto typeSubscription = makeNamedValidateType(service::ObjectType { "Subscription" });
		auto typeAppointment = makeNamedValidateType(service::ObjectType { "Appointment" });
		auto typeTask = makeNamedValidateType(service::ObjectType { "Task" });
		auto typeFolder = makeNamedValidateType(service::ObjectType { "Folder" });
		auto typeNestedType = makeNamedValidateType(service::ObjectType { "NestedType" });
		auto typeExpensive = makeNamedValidateType(service::ObjectType { "Expensive" });

		typeCompleteTaskInput->setFields({
				{ "id", { makeNonNullOfType(typeID), 0, 0 } },
				{ "isComplete", { typeBoolean, 1, 1 } },
				{ "clientMutationId", { typeString, 0, 0 } }
			});

		typeUnionType->setPossibleTypes({
				typeAppointment.get(),
				typeTask.get(),
				typeFolder.get()
			});


		typeNode->setPossibleTypes({
				typeAppointment.get(),
				typeTask.get(),
				typeFolder.get()
			});
		typeNode->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } }
			});

		typeQuery->setFields({
				{ "node", { typeNode, { { "id", { makeNonNullOfType(typeID), 0, 0 } } } } },
				{ "appointments", { makeNonNullOfType(typeAppointmentConnection), { { "first", { typeInt, 0, 0 } }, { "after", { typeItemCursor, 0, 0 } }, { "last", { typeInt, 0, 0 } }, { "before", { typeItemCursor, 0, 0 } } } } },
				{ "tasks", { makeNonNullOfType(typeTaskConnection), { { "first", { typeInt, 0, 0 } }, { "after", { typeItemCursor, 0, 0 } }, { "last", { typeInt, 0, 0 } }, { "before", { typeItemCursor, 0, 0 } } } } },
				{ "unreadCounts", { makeNonNullOfType(typeFolderConnection), { { "first", { typeInt, 0, 0 } }, { "after", { typeItemCursor, 0, 0 } }, { "last", { typeInt, 0, 0 } }, { "before", { typeItemCursor, 0, 0 } } } } },
				{ "appointmentsById", { makeNonNullOfType(makeListOfType(typeAppointment)), { { "ids", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeID))), 1, 1 } } } } },
				{ "tasksById", { makeNonNullOfType(makeListOfType(typeTask)), { { "ids", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeID))), 0, 0 } } } } },
				{ "unreadCountsById", { makeNonNullOfType(makeListOfType(typeFolder)), { { "ids", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeID))), 0, 0 } } } } },
				{ "nested", { makeNonNullOfType(typeNestedType), {  } } },
				{ "unimplemented", { makeNonNullOfType(typeString), {  } } },
				{ "expensive", { makeNonNullOfType(makeListOfType(makeNonNullOfType(typeExpensive))), {  } } }
			});
		typePageInfo->setFields({
				{ "hasNextPage", { makeNonNullOfType(typeBoolean), {  } } },
				{ "hasPreviousPage", { makeNonNullOfType(typeBoolean), {  } } }
			});
		typeAppointmentEdge->setFields({
				{ "node", { typeAppointment, {  } } },
				{ "cursor", { makeNonNullOfType(typeItemCursor), {  } } }
			});
		typeAppointmentConnection->setFields({
				{ "pageInfo", { makeNonNullOfType(typePageInfo), {  } } },
				{ "edges", { makeListOfType(typeAppointmentEdge), {  } } }
			});
		typeTaskEdge->setFields({
				{ "node", { typeTask, {  } } },
				{ "cursor", { makeNonNullOfType(typeItemCursor), {  } } }
			});
		typeTaskConnection->setFields({
				{ "pageInfo", { makeNonNullOfType(typePageInfo), {  } } },
				{ "edges", { makeListOfType(typeTaskEdge), {  } } }
			});
		typeFolderEdge->setFields({
				{ "node", { typeFolder, {  } } },
				{ "cursor", { makeNonNullOfType(typeItemCursor), {  } } }
			});
		typeFolderConnection->setFields({
				{ "pageInfo", { makeNonNullOfType(typePageInfo), {  } } },
				{ "edges", { makeListOfType(typeFolderEdge), {  } } }
			});
		typeCompleteTaskPayload->setFields({
				{ "task", { typeTask, {  } } },
				{ "clientMutationId", { typeString, {  } } }
			});
		typeMutation->setFields({
				{ "completeTask", { makeNonNullOfType(typeCompleteTaskPayload), { { "input", { makeNonNullOfType(typeCompleteTaskInput), 0, 0 } } } } },
				{ "setFloat", { makeNonNullOfType(typeFloat), { { "value", { makeNonNullOfType(typeFloat), 0, 0 } } } } }
			});
		typeSubscription->setFields({
				{ "nextAppointmentChange", { typeAppointment, {  } } },
				{ "nodeChange", { makeNonNullOfType(typeNode), { { "id", { makeNonNullOfType(typeID), 0, 0 } } } } }
			});
		typeAppointment->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } },
				{ "when", { typeDateTime, {  } } },
				{ "subject", { typeString, {  } } },
				{ "isNow", { makeNonNullOfType(typeBoolean), {  } } }
			});
		typeTask->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } },
				{ "title", { typeString, {  } } },
				{ "isComplete", { makeNonNullOfType(typeBoolean), {  } } }
			});
		typeFolder->setFields({
				{ "id", { makeNonNullOfType(typeID), {  } } },
				{ "name", { typeString, {  } } },
				{ "unreadCount", { makeNonNullOfType(typeInt), {  } } }
			});
		typeNestedType->setFields({
				{ "depth", { makeNonNullOfType(typeInt), {  } } },
				{ "nested", { makeNonNullOfType(typeNestedType), {  } } }
			});
		typeExpensive->setFields({
				{ "order", { makeNonNullOfType(typeInt), {  } } }
			});


		_directives = {
			{ "id", { { introspection::DirectiveLocation::FIELD_DEFINITION }, {  } } },
			{ "subscriptionTag", { { introspection::DirectiveLocation::SUBSCRIPTION }, { { "field", { typeString, 0, 0 } } } } },
			{ "queryTag", { { introspection::DirectiveLocation::QUERY }, { { "query", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "fieldTag", { { introspection::DirectiveLocation::FIELD }, { { "field", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "fragmentDefinitionTag", { { introspection::DirectiveLocation::FRAGMENT_DEFINITION }, { { "fragmentDefinition", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "fragmentSpreadTag", { { introspection::DirectiveLocation::FRAGMENT_SPREAD }, { { "fragmentSpread", { makeNonNullOfType(typeString), 0, 0 } } } } },
			{ "inlineFragmentTag", { { introspection::DirectiveLocation::INLINE_FRAGMENT }, { { "inlineFragment", { makeNonNullOfType(typeString), 0, 0 } } } } }
		};

		_operationTypes.queryType = "Query";
		_operationTypes.mutationType = "Mutation";
		_operationTypes.subscriptionType = "Subscription";

	}
};


Operations::Operations(std::shared_ptr<object::Query> query, std::shared_ptr<object::Mutation> mutation, std::shared_ptr<object::Subscription> subscription)
	: service::Request({
		{ "query", query },
		{ "mutation", mutation },
		{ "subscription", subscription }
	}, std::make_unique<ValidationContext>())
	, _query(std::move(query))
	, _mutation(std::move(mutation))
	, _subscription(std::move(subscription))
{
}

void AddTypesToSchema(const std::shared_ptr<introspection::Schema>& schema)
{
	schema->AddType("ItemCursor", std::make_shared<introspection::ScalarType>("ItemCursor", R"md()md"));
	schema->AddType("DateTime", std::make_shared<introspection::ScalarType>("DateTime", R"md()md"));
	auto typeTaskState = std::make_shared<introspection::EnumType>("TaskState", R"md()md");
	schema->AddType("TaskState", typeTaskState);
	auto typeCompleteTaskInput = std::make_shared<introspection::InputObjectType>("CompleteTaskInput", R"md()md");
	schema->AddType("CompleteTaskInput", typeCompleteTaskInput);
	auto typeUnionType = std::make_shared<introspection::UnionType>("UnionType", R"md()md");
	schema->AddType("UnionType", typeUnionType);
	auto typeNode = std::make_shared<introspection::InterfaceType>("Node", R"md(Node interface for Relay support)md");
	schema->AddType("Node", typeNode);
	auto typeQuery = std::make_shared<introspection::ObjectType>("Query", R"md(Root Query type)md");
	schema->AddType("Query", typeQuery);
	auto typePageInfo = std::make_shared<introspection::ObjectType>("PageInfo", R"md()md");
	schema->AddType("PageInfo", typePageInfo);
	auto typeAppointmentEdge = std::make_shared<introspection::ObjectType>("AppointmentEdge", R"md()md");
	schema->AddType("AppointmentEdge", typeAppointmentEdge);
	auto typeAppointmentConnection = std::make_shared<introspection::ObjectType>("AppointmentConnection", R"md()md");
	schema->AddType("AppointmentConnection", typeAppointmentConnection);
	auto typeTaskEdge = std::make_shared<introspection::ObjectType>("TaskEdge", R"md()md");
	schema->AddType("TaskEdge", typeTaskEdge);
	auto typeTaskConnection = std::make_shared<introspection::ObjectType>("TaskConnection", R"md()md");
	schema->AddType("TaskConnection", typeTaskConnection);
	auto typeFolderEdge = std::make_shared<introspection::ObjectType>("FolderEdge", R"md()md");
	schema->AddType("FolderEdge", typeFolderEdge);
	auto typeFolderConnection = std::make_shared<introspection::ObjectType>("FolderConnection", R"md()md");
	schema->AddType("FolderConnection", typeFolderConnection);
	auto typeCompleteTaskPayload = std::make_shared<introspection::ObjectType>("CompleteTaskPayload", R"md()md");
	schema->AddType("CompleteTaskPayload", typeCompleteTaskPayload);
	auto typeMutation = std::make_shared<introspection::ObjectType>("Mutation", R"md()md");
	schema->AddType("Mutation", typeMutation);
	auto typeSubscription = std::make_shared<introspection::ObjectType>("Subscription", R"md()md");
	schema->AddType("Subscription", typeSubscription);
	auto typeAppointment = std::make_shared<introspection::ObjectType>("Appointment", R"md()md");
	schema->AddType("Appointment", typeAppointment);
	auto typeTask = std::make_shared<introspection::ObjectType>("Task", R"md()md");
	schema->AddType("Task", typeTask);
	auto typeFolder = std::make_shared<introspection::ObjectType>("Folder", R"md()md");
	schema->AddType("Folder", typeFolder);
	auto typeNestedType = std::make_shared<introspection::ObjectType>("NestedType", R"md(Infinitely nestable type which can be used with nested fragments to test directive handling)md");
	schema->AddType("NestedType", typeNestedType);
	auto typeExpensive = std::make_shared<introspection::ObjectType>("Expensive", R"md()md");
	schema->AddType("Expensive", typeExpensive);

	typeTaskState->AddEnumValues({
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::New)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Started)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Complete)] }, R"md()md", std::nullopt },
		{ std::string{ service::s_namesTaskState[static_cast<size_t>(today::TaskState::Unassigned)] }, R"md()md", std::make_optional<response::StringType>(R"md(Need to deprecate an [enum value](https://facebook.github.io/graphql/June2018/#sec-Deprecation))md") }
	});

	typeCompleteTaskInput->AddInputValues({
		std::make_shared<introspection::InputValue>("id", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")), R"gql()gql"),
		std::make_shared<introspection::InputValue>("isComplete", R"md()md", schema->LookupType("Boolean"), R"gql(true)gql"),
		std::make_shared<introspection::InputValue>("clientMutationId", R"md()md", schema->LookupType("String"), R"gql()gql")
	});

	typeUnionType->AddPossibleTypes({
		schema->LookupType("Appointment"),
		schema->LookupType("Task"),
		schema->LookupType("Folder")
	});

	typeNode->AddFields({
		std::make_shared<introspection::Field>("id", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>(), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("ID")))
	});

	AddQueryDetails(typeQuery, schema);
	AddPageInfoDetails(typePageInfo, schema);
	AddAppointmentEdgeDetails(typeAppointmentEdge, schema);
	AddAppointmentConnectionDetails(typeAppointmentConnection, schema);
	AddTaskEdgeDetails(typeTaskEdge, schema);
	AddTaskConnectionDetails(typeTaskConnection, schema);
	AddFolderEdgeDetails(typeFolderEdge, schema);
	AddFolderConnectionDetails(typeFolderConnection, schema);
	AddCompleteTaskPayloadDetails(typeCompleteTaskPayload, schema);
	AddMutationDetails(typeMutation, schema);
	AddSubscriptionDetails(typeSubscription, schema);
	AddAppointmentDetails(typeAppointment, schema);
	AddTaskDetails(typeTask, schema);
	AddFolderDetails(typeFolder, schema);
	AddNestedTypeDetails(typeNestedType, schema);
	AddExpensiveDetails(typeExpensive, schema);

	schema->AddDirective(std::make_shared<introspection::Directive>("id", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD_DEFINITION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>()));
	schema->AddDirective(std::make_shared<introspection::Directive>("subscriptionTag", R"md()md", std::vector<response::StringType>({
		R"gql(SUBSCRIPTION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("field", R"md()md", schema->LookupType("String"), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("queryTag", R"md()md", std::vector<response::StringType>({
		R"gql(QUERY)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("query", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fieldTag", R"md()md", std::vector<response::StringType>({
		R"gql(FIELD)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("field", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fragmentDefinitionTag", R"md()md", std::vector<response::StringType>({
		R"gql(FRAGMENT_DEFINITION)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("fragmentDefinition", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("fragmentSpreadTag", R"md()md", std::vector<response::StringType>({
		R"gql(FRAGMENT_SPREAD)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("fragmentSpread", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));
	schema->AddDirective(std::make_shared<introspection::Directive>("inlineFragmentTag", R"md()md", std::vector<response::StringType>({
		R"gql(INLINE_FRAGMENT)gql"
	}), std::vector<std::shared_ptr<introspection::InputValue>>({
		std::make_shared<introspection::InputValue>("inlineFragment", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("String")), R"gql()gql")
	})));

	schema->AddQueryType(typeQuery);
	schema->AddMutationType(typeMutation);
	schema->AddSubscriptionType(typeSubscription);
}

} /* namespace today */
} /* namespace graphql */
