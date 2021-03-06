// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "TodayObjects.h"

#include "graphqlservice/Introspection.h"

#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

using namespace std::literals;

namespace graphql::today {
namespace object {

Mutation::Mutation()
	: service::Object({
		"Mutation"
	}, {
		{ R"gql(__typename)gql"sv, [this](service::ResolverParams&& params) { return resolve_typename(std::move(params)); } },
		{ R"gql(completeTask)gql"sv, [this](service::ResolverParams&& params) { return resolveCompleteTask(std::move(params)); } },
		{ R"gql(setFloat)gql"sv, [this](service::ResolverParams&& params) { return resolveSetFloat(std::move(params)); } }
	})
{
}

service::FieldResult<std::shared_ptr<CompleteTaskPayload>> Mutation::applyCompleteTask(service::FieldParams&&, CompleteTaskInput&&) const
{
	throw std::runtime_error(R"ex(Mutation::applyCompleteTask is not implemented)ex");
}

std::future<response::Value> Mutation::resolveCompleteTask(service::ResolverParams&& params)
{
	auto argInput = service::ModifiedArgument<today::CompleteTaskInput>::require("input", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = applyCompleteTask(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argInput));
	resolverLock.unlock();

	return service::ModifiedResult<CompleteTaskPayload>::convert(std::move(result), std::move(params));
}

service::FieldResult<response::FloatType> Mutation::applySetFloat(service::FieldParams&&, response::FloatType&&) const
{
	throw std::runtime_error(R"ex(Mutation::applySetFloat is not implemented)ex");
}

std::future<response::Value> Mutation::resolveSetFloat(service::ResolverParams&& params)
{
	auto argValue = service::ModifiedArgument<response::FloatType>::require("value", params.arguments);
	std::unique_lock resolverLock(_resolverMutex);
	auto result = applySetFloat(service::FieldParams(params, std::move(params.fieldDirectives)), std::move(argValue));
	resolverLock.unlock();

	return service::ModifiedResult<response::FloatType>::convert(std::move(result), std::move(params));
}

std::future<response::Value> Mutation::resolve_typename(service::ResolverParams&& params)
{
	return service::ModifiedResult<response::StringType>::convert(response::StringType{ R"gql(Mutation)gql" }, std::move(params));
}

} /* namespace object */

void AddMutationDetails(std::shared_ptr<introspection::ObjectType> typeMutation, const std::shared_ptr<introspection::Schema>& schema)
{
	typeMutation->AddFields({
		std::make_shared<introspection::Field>("completeTask", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("input", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("CompleteTaskInput")), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("CompleteTaskPayload"))),
		std::make_shared<introspection::Field>("setFloat", R"md()md", std::nullopt, std::vector<std::shared_ptr<introspection::InputValue>>({
			std::make_shared<introspection::InputValue>("value", R"md()md", schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Float")), R"gql()gql")
		}), schema->WrapType(introspection::TypeKind::NON_NULL, schema->LookupType("Float")))
	});
}

} /* namespace graphql::today */
