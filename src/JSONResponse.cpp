// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "graphqlservice/JSONResponse.h"

#define RAPIDJSON_NAMESPACE graphql::rapidjson
#include <rapidjson/rapidjson.h>

#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <limits>
#include <stack>
#include <stdexcept>

class StringOwnerBuffer
{
public:
	typedef char Ch;

	StringOwnerBuffer(std::string& storage)
		: _storage(storage)
	{
	}

	void Reserve(size_t count)
	{
		_storage.reserve(_storage.length() + count);
	}

	void PutN(char c, size_t n)
	{
		_storage.reserve(_storage.length() + n);
		for (size_t i = 0; i < n; i++)
		{
			_storage.push_back(c);
		}
	}

	void Put(char c)
	{
		_storage.push_back(c);
	}

	void Clear()
	{
		_storage.clear();
	}

	void Flush()
	{
		return;
	}
	size_t Size() const
	{
		return _storage.length();
	}

private:
	std::string& _storage;
};

inline void PutReserve(StringOwnerBuffer& stream, size_t count)
{
	stream.Reserve(count);
}

inline void PutN(StringOwnerBuffer& stream, char c, size_t n)
{
	stream.PutN(c, n);
}

namespace graphql::response {

void writeResponse(rapidjson::Writer<StringOwnerBuffer>& writer, Value&& response)
{
	switch (response.type())
	{
		case Type::Map:
		{
			auto members = response.release<MapType>();

			writer.StartObject();

			for (auto& entry : members)
			{
				writer.Key(entry.first.c_str());
				writeResponse(writer, std::move(entry.second));
			}

			writer.EndObject();
			break;
		}

		case Type::List:
		{
			auto elements = response.release<ListType>();

			writer.StartArray();

			for (auto& entry : elements)
			{
				writeResponse(writer, std::move(entry));
			}

			writer.EndArray();
			break;
		}

		case Type::String:
		case Type::EnumValue:
		{
			auto value = response.release<StringType>();

			writer.String(value.c_str());
			break;
		}

		case Type::Null:
		{
			writer.Null();
			break;
		}

		case Type::Boolean:
		{
			writer.Bool(response.get<BooleanType>());
			break;
		}

		case Type::Int:
		{
			writer.Int(response.get<IntType>());
			break;
		}

		case Type::Float:
		{
			writer.Double(response.get<FloatType>());
			break;
		}

		case Type::Scalar:
		{
			writeResponse(writer, response.release<ScalarType>());
			break;
		}

		default:
		{
			writer.Null();
			break;
		}
	}
}

std::string toJSON(Value&& response, size_t reserved)
{
	std::string storage;
	storage.reserve(reserved);

	StringOwnerBuffer buffer(storage);
	rapidjson::Writer writer(buffer);

	writeResponse(writer, std::move(response));

	return std::move(storage);
}

struct ResponseHandler : rapidjson::BaseReaderHandler<rapidjson::UTF8<>, ResponseHandler>
{
	ResponseHandler()
	{
		// Start with a single null value.
		_responseStack.push({});
	}

	Value getResponse()
	{
		auto response = std::move(_responseStack.top());

		_responseStack.pop();

		return response;
	}

	bool Null()
	{
		setValue(Value());
		return true;
	}

	bool Bool(bool b)
	{
		setValue(Value(b));
		return true;
	}

	bool Int(int i)
	{
		// https://facebook.github.io/graphql/June2018/#sec-Int
		static_assert(sizeof(i) == 4, "GraphQL only supports 32-bit signed integers");
		auto value = Value(Type::Int);

		value.set<IntType>(std::move(i));
		setValue(std::move(value));
		return true;
	}

	bool Uint(unsigned int i)
	{
		if (i > static_cast<unsigned int>(std::numeric_limits<int>::max()))
		{
			// https://facebook.github.io/graphql/June2018/#sec-Int
			throw std::overflow_error("GraphQL only supports 32-bit signed integers");
		}
		return Int(static_cast<int>(i));
	}

	bool Int64(int64_t /*i*/)
	{
		// https://facebook.github.io/graphql/June2018/#sec-Int
		throw std::overflow_error("GraphQL only supports 32-bit signed integers");
	}

	bool Uint64(uint64_t /*i*/)
	{
		// https://facebook.github.io/graphql/June2018/#sec-Int
		throw std::overflow_error("GraphQL only supports 32-bit signed integers");
	}

	bool Double(double d)
	{
		auto value = Value(Type::Float);

		value.set<FloatType>(std::move(d));
		setValue(std::move(value));
		return true;
	}

	bool String(const Ch* str, rapidjson::SizeType /*length*/, bool /*copy*/)
	{
		setValue(Value(std::string(str)).from_json());
		return true;
	}

	bool StartObject()
	{
		_responseStack.push(Value(Type::Map));
		return true;
	}

	bool Key(const Ch* str, rapidjson::SizeType /*length*/, bool /*copy*/)
	{
		_keyStack.push(str);
		return true;
	}

	bool EndObject(rapidjson::SizeType /*count*/)
	{
		setValue(getResponse());
		return true;
	}

	bool StartArray()
	{
		_responseStack.push(Value(Type::List));
		return true;
	}

	bool EndArray(rapidjson::SizeType /*count*/)
	{
		setValue(getResponse());
		return true;
	}

private:
	void setValue(Value&& value)
	{
		switch (_responseStack.top().type())
		{
			case Type::Map:
				_responseStack.top().emplace_back(std::move(_keyStack.top()), std::move(value));
				_keyStack.pop();
				break;

			case Type::List:
				_responseStack.top().emplace_back(std::move(value));
				break;

			default:
				_responseStack.top() = std::move(value);
				break;
		}
	}

	std::stack<std::string> _keyStack;
	std::stack<Value> _responseStack;
};

Value parseJSON(const std::string_view& json)
{
	ResponseHandler handler;
	rapidjson::Reader reader;
	rapidjson::MemoryStream ss(json.data(), json.size());

	reader.Parse(ss, handler);

	return handler.getResponse();
}

} /* namespace graphql::response */
