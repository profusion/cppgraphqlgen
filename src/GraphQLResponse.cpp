// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/GraphQLResponse.h"

#include <optional>
#include <stdexcept>
#include <variant>

namespace graphql::response {

// Type::Map
struct MapData
{
	bool operator==(const MapData& rhs) const
	{
		return map == rhs.map;
	}

	MapType map;
	std::unordered_map<std::string, size_t> members;
};

// Type::List
struct ListData
{
	bool operator==(const ListData& rhs) const
	{
		return list == rhs.list;
	}

	ListType list;
};

// Type::String or Type::EnumValue
struct StringOrEnumData
{
	bool operator==(const StringOrEnumData& rhs) const
	{
		return string == rhs.string && from_json == rhs.from_json;
	}

	StringType string;
	bool from_json = false;
};

// Type::Scalar
struct ScalarData
{
	bool operator==(const ScalarData& rhs) const
	{
		return scalar == rhs.scalar;
	}

	ScalarType scalar;
};

struct TypedData
	: std::variant<std::optional<MapData>, std::optional<ListData>, std::optional<StringOrEnumData>,
		  std::optional<ScalarData>, BooleanType, IntType, FloatType, std::optional<ResultType>>
{
};

Value::Value(Type type /*= Type::Null*/)
	: _type(type)
{
	switch (type)
	{
		case Type::Map:
			_data = std::make_unique<TypedData>(TypedData { std::make_optional<MapData>() });
			break;

		case Type::List:
			_data = std::make_unique<TypedData>(TypedData { std::make_optional<ListData>() });
			break;

		case Type::String:
		case Type::EnumValue:
			_data =
				std::make_unique<TypedData>(TypedData { std::make_optional<StringOrEnumData>() });
			break;

		case Type::Scalar:
			_data = std::make_unique<TypedData>(TypedData { std::make_optional<ScalarData>() });
			break;

		case Type::Boolean:
			_data = std::make_unique<TypedData>(TypedData { BooleanType { false } });
			break;

		case Type::Int:
			_data = std::make_unique<TypedData>(TypedData { IntType { 0 } });
			break;

		case Type::Float:
			_data = std::make_unique<TypedData>(TypedData { FloatType { 0.0 } });
			break;

		case Type::Result:
			_data = std::make_unique<TypedData>(TypedData { std::make_optional<ResultType>() });

		default:
			break;
	}
}

Value::~Value()
{
	// The default destructor gets inlined and may use a different allocator to free Value's member
	// variables than the graphqlservice module used to allocate them. So even though this could be
	// omitted, declare it explicitly and define it in graphqlservice.
}

Value::Value(const char* value)
	: _type(Type::String)
	, _data(std::make_unique<TypedData>(
		  TypedData { StringOrEnumData { StringType { value }, false } }))
{
}

Value::Value(StringType&& value)
	: _type(Type::String)
	, _data(std::make_unique<TypedData>(TypedData { StringOrEnumData { std::move(value), false } }))
{
}

Value::Value(BooleanType value)
	: _type(Type::Boolean)
	, _data(std::make_unique<TypedData>(TypedData { value }))
{
}

Value::Value(IntType value)
	: _type(Type::Int)
	, _data(std::make_unique<TypedData>(TypedData { value }))
{
}

Value::Value(FloatType value)
	: _type(Type::Float)
	, _data(std::make_unique<TypedData>(TypedData { value }))
{
}

Value::Value(ResultType&& value)
	: _type(Type::Result)
	, _data(std::make_unique<TypedData>(TypedData { ResultType { std::move(value) } }))
{
}

Value::Value(Value&& other) noexcept
	: _type(other.type())
	, _data(std::move(other._data))
{
}

Value::Value(const Value& other)
	: _type(other.type())
	, _data(std::make_unique<TypedData>(other._data ? *other._data : TypedData {}))
{
}

Value& Value::operator=(Value&& rhs) noexcept
{
	if (&rhs != this)
	{
		const_cast<Type&>(_type) = rhs._type;
		_data = std::move(rhs._data);
	}

	return *this;
}

bool Value::operator==(const Value& rhs) const noexcept
{
	if (rhs.type() != type())
	{
		return false;
	}

	return !_data || *_data == *rhs._data;
}

bool Value::operator!=(const Value& rhs) const noexcept
{
	return !(*this == rhs);
}

Type Value::type() const noexcept
{
	return _data ? _type : Type::Null;
}

Value&& Value::from_json() noexcept
{
	std::get<std::optional<StringOrEnumData>>(*_data)->from_json = true;

	return std::move(*this);
}

bool Value::maybe_enum() const noexcept
{
	return type() == Type::EnumValue
		|| (type() == Type::String && std::get<std::optional<StringOrEnumData>>(*_data)->from_json);
}

void Value::reserve(size_t count)
{
	switch (type())
	{
		case Type::Map:
		{
			auto& mapData = std::get<std::optional<MapData>>(*_data);

			mapData->members.reserve(count);
			mapData->map.reserve(count);
			break;
		}

		case Type::List:
		{
			auto& listData = std::get<std::optional<ListData>>(*_data);

			listData->list.reserve(count);
			break;
		}

		default:
			throw std::logic_error("Invalid call to Value::reserve");
	}
}

size_t Value::size() const
{
	switch (type())
	{
		case Type::Map:
		{
			const auto& mapData = std::get<std::optional<MapData>>(*_data);

			return mapData->map.size();
		}

		case Type::List:
		{
			const auto& listData = std::get<std::optional<ListData>>(*_data);

			return listData->list.size();
		}

		case Type::Result:
		{
			const auto& resultData = std::get<std::optional<ResultType>>(*_data);
			return resultData->size();
		}

		default:
			throw std::logic_error("Invalid call to Value::size");
	}
}

void Value::emplace_back(std::string&& name, Value&& value)
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for MapType");
	}

	auto& mapData = std::get<std::optional<MapData>>(*_data);

	if (mapData->members.find(name) != mapData->members.cend())
	{
		throw std::runtime_error("Duplicate Map member");
	}

	mapData->members.insert({ name, mapData->map.size() });
	mapData->map.emplace_back(std::make_pair(std::move(name), std::move(value)));
}

MapType::const_iterator Value::find(const std::string& name) const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::find for MapType");
	}

	const auto& mapData = std::get<std::optional<MapData>>(*_data);
	const auto itr = mapData->members.find(name);

	if (itr == mapData->members.cend())
	{
		return mapData->map.cend();
	}

	return mapData->map.cbegin() + itr->second;
}

MapType::const_iterator Value::begin() const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	return std::get<std::optional<MapData>>(*_data)->map.cbegin();
}

MapType::const_iterator Value::end() const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::end for MapType");
	}

	return std::get<std::optional<MapData>>(*_data)->map.cend();
}

const Value& Value::operator[](const std::string& name) const
{
	const auto itr = find(name);

	if (itr == end())
	{
		throw std::runtime_error("Missing Map member");
	}

	return itr->second;
}

void Value::emplace_back(Value&& value)
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	std::get<std::optional<ListData>>(*_data)->list.emplace_back(std::move(value));
}

const Value& Value::operator[](size_t index) const
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::emplace_back for ListType");
	}

	return std::get<std::optional<ListData>>(*_data)->list.at(index);
}

template <>
void Value::set<StringType>(StringType&& value)
{
	if (type() != Type::String && type() != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::set for StringType");
	}

	std::get<std::optional<StringOrEnumData>>(*_data)->string = std::move(value);
}

template <>
void Value::set<BooleanType>(BooleanType value)
{
	if (type() != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::set for BooleanType");
	}

	*_data = { value };
}

template <>
void Value::set<IntType>(IntType value)
{
	if (type() == Type::Float)
	{
		// Coerce IntType to FloatType
		*_data = { static_cast<FloatType>(value) };
	}
	else
	{
		if (type() != Type::Int)
		{
			throw std::logic_error("Invalid call to Value::set for IntType");
		}

		*_data = { value };
	}
}

template <>
void Value::set<FloatType>(FloatType value)
{
	if (type() != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::set for FloatType");
	}

	*_data = { value };
}

template <>
void Value::set<ScalarType>(ScalarType&& value)
{
	if (type() != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::set for ScalarType");
	}

	*_data = { ScalarData { std::move(value) } };
}

template <>
const MapType& Value::get<MapType>() const
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::get for MapType");
	}

	return std::get<std::optional<MapData>>(*_data)->map;
}

template <>
const ListType& Value::get<ListType>() const
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::get for ListType");
	}

	return std::get<std::optional<ListData>>(*_data)->list;
}

template <>
const StringType& Value::get<StringType>() const
{
	if (type() != Type::String && type() != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::get for StringType");
	}

	return std::get<std::optional<StringOrEnumData>>(*_data)->string;
}

template <>
BooleanType Value::get<BooleanType>() const
{
	if (type() != Type::Boolean)
	{
		throw std::logic_error("Invalid call to Value::get for BooleanType");
	}

	return std::get<BooleanType>(*_data);
}

template <>
IntType Value::get<IntType>() const
{
	if (type() != Type::Int)
	{
		throw std::logic_error("Invalid call to Value::get for IntType");
	}

	return std::get<IntType>(*_data);
}

template <>
FloatType Value::get<FloatType>() const
{
	if (type() == Type::Int)
	{
		// Coerce IntType to FloatType
		return static_cast<FloatType>(std::get<IntType>(*_data));
	}

	if (type() != Type::Float)
	{
		throw std::logic_error("Invalid call to Value::get for FloatType");
	}

	return std::get<FloatType>(*_data);
}

template <>
const ScalarType& Value::get<ScalarType>() const
{
	if (type() != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::get for ScalarType");
	}

	return std::get<std::optional<ScalarData>>(*_data)->scalar;
}

template <>
const ResultType& Value::get<ResultType>() const
{
	if (type() != Type::Result)
	{
		throw std::logic_error("Invalid call to Value::get for ResultType");
	}

	return *std::get<std::optional<ResultType>>(*_data);
}

template <>
MapType Value::release<MapType>()
{
	if (type() != Type::Map)
	{
		throw std::logic_error("Invalid call to Value::release for MapType");
	}

	auto& mapData = std::get<std::optional<MapData>>(*_data);
	MapType result = std::move(mapData->map);

	mapData->members.clear();

	return result;
}

template <>
ListType Value::release<ListType>()
{
	if (type() != Type::List)
	{
		throw std::logic_error("Invalid call to Value::release for ListType");
	}

	ListType result = std::move(std::get<std::optional<ListData>>(*_data)->list);

	return result;
}

template <>
StringType Value::release<StringType>()
{
	if (type() != Type::String && type() != Type::EnumValue)
	{
		throw std::logic_error("Invalid call to Value::release for StringType");
	}

	auto& stringData = std::get<std::optional<StringOrEnumData>>(*_data);
	StringType result = std::move(stringData->string);

	stringData->from_json = false;

	return result;
}

template <>
ScalarType Value::release<ScalarType>()
{
	if (type() != Type::Scalar)
	{
		throw std::logic_error("Invalid call to Value::release for ScalarType");
	}

	ScalarType result = std::move(std::get<std::optional<ScalarData>>(*_data)->scalar);

	return result;
}

template <>
ResultType Value::release<ResultType>()
{
	if (type() != Type::Result)
	{
		throw std::logic_error("Invalid call to Value::release for ResultType");
	}

	ResultType result = std::move(*std::get<std::optional<ResultType>>(*_data));

	return result;
}

Value Value::toMap()
{
	if (type() != Type::Result)
	{
		throw std::logic_error("Invalid call to Value::toMap for ResultType");
	}

	auto resultData = release<ResultType>();

	Value map(Type::Map);
	map.reserve(resultData.size());
	map.emplace_back(std::string { strData }, std::move(resultData.data));
	if (resultData.errors.size() > 0)
	{
		map.emplace_back(std::string { strErrors }, buildErrorValues(resultData.errors));
	}

	return map;
}

size_t ResultType::size() const
{
	return 1 + (errors.size() > 0);
}

void addErrorMessage(std::string&& message, Value& error)
{
	error.emplace_back(std::string { strMessage }, response::Value(std::move(message)));
}

void addErrorLocation(const graphql::error::schema_location& location, response::Value& error)
{
	if (location == graphql::error::emptyLocation)
	{
		return;
	}

	response::Value errorLocation(response::Type::Map);

	errorLocation.reserve(2);
	errorLocation.emplace_back(std::string { strLine },
		response::Value(static_cast<response::IntType>(location.line)));
	errorLocation.emplace_back(std::string { strColumn },
		response::Value(static_cast<response::IntType>(location.column)));

	response::Value errorLocations(response::Type::List);

	errorLocations.reserve(1);
	errorLocations.emplace_back(std::move(errorLocation));

	error.emplace_back(std::string { strLocations }, std::move(errorLocations));
}

void addErrorPath(graphql::error::field_path&& path, Value& error)
{
	if (path.empty())
	{
		return;
	}

	response::Value errorPath(response::Type::List);

	errorPath.reserve(path.size());
	while (!path.empty())
	{
		auto& segment = path.front();

		if (std::holds_alternative<std::string>(segment))
		{
			errorPath.emplace_back(response::Value(std::move(std::get<std::string>(segment))));
		}
		else if (std::holds_alternative<size_t>(segment))
		{
			errorPath.emplace_back(
				response::Value(static_cast<response::IntType>(std::get<size_t>(segment))));
		}

		path.pop();
	}

	error.emplace_back(std::string { strPath }, std::move(errorPath));
}

response::Value buildErrorValues(const std::vector<graphql::error::schema_error>& structuredErrors)
{
	response::Value errors(response::Type::List);

	errors.reserve(structuredErrors.size());

	for (auto error : structuredErrors)
	{
		response::Value entry(response::Type::Map);

		entry.reserve(3);
		addErrorMessage(std::move(error.message), entry);
		addErrorLocation(error.location, entry);
		addErrorPath(std::move(error.path), entry);

		errors.emplace_back(std::move(entry));
	}

	return errors;
}

} /* namespace graphql::response */
