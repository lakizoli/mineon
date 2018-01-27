#include "stdafx.h"
#include "Json.hpp"
#include <rapidjson/memorystream.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/memorybuffer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/writer.h>

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////
// JSONBase
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ResultT, class ContainerT>
shared_ptr<ResultT> JSONBase::Parse (const ContainerT& jsonUTF8) {
	using namespace rapidjson;

	if (jsonUTF8.size () <= 0) {
		return nullptr;
	}

	//Parse JSON
	MemoryStream ms ((const MemoryStream::Ch*) &jsonUTF8[0], jsonUTF8.size ());
	EncodedInputStream<UTF8<>, MemoryStream> stream (ms);

	shared_ptr<Document> doc = make_shared<Document> ();
	doc->ParseStream (stream);
	if (doc->HasParseError ()) {
		return nullptr;
	}

	//Convert to API objects
	shared_ptr<JSONBase> result;
	if (doc->IsArray()) { //Array result
		result = shared_ptr<JSONArray> (new JSONArray (doc));
	} else if (doc->IsObject ()) { //Object result
		result = shared_ptr<JSONObject> (new JSONObject (doc));
	}

	return dynamic_pointer_cast<ResultT> (result);
}

template<typename Encoding, typename Allocator>
JSONDataType JSONBase::ValueAsString (const rapidjson::GenericValue<Encoding, Allocator>& value, string& jsonValue) const {
	JSONDataType type = JSONDataType::Unknown;
	if (value.IsString ()) {
		type = JSONDataType::String;
		jsonValue = value.GetString ();
	} else if (value.IsBool ()) {
		type = JSONDataType::Bool;
		jsonValue = value.GetBool () ? "true" : "false";
	} else if (value.IsInt ()) {
		type = JSONDataType::Int32;
		jsonValue = to_string (value.GetInt ());
	} else if (value.IsInt64 ()) {
		type = JSONDataType::Int64;
		jsonValue = to_string (value.GetInt64 ());
	} else if (value.IsUint ()) {
		type = JSONDataType::UInt32;
		jsonValue = to_string (value.GetUint ());
	} else if (value.IsUint64 ()) {
		type = JSONDataType::UInt64;
		jsonValue = to_string (value.GetUint64 ());
	} else if (value.IsDouble ()) {
		type = JSONDataType::Double;
		jsonValue = to_string (value.GetDouble ());
	} else if (value.IsObject () || value.IsArray ()) {
		type = value.IsArray () ? JSONDataType::Array : JSONDataType::Object;

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
		value.Accept (writer);
		jsonValue = buffer.GetString ();
	}

	return type;
}

string JSONBase::ToString (bool prettyPrint) const {
	rapidjson::StringBuffer buffer;
	if (prettyPrint) {
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer (buffer);
		mDocument->Accept (writer);
	} else {
		rapidjson::Writer<rapidjson::StringBuffer> writer (buffer);
		mDocument->Accept (writer);
	}
	return buffer.GetString ();
}

vector<uint8_t> JSONBase::ToVector () const {
	rapidjson::MemoryBuffer buffer;
	rapidjson::Writer<rapidjson::MemoryBuffer> writer (buffer);
	mDocument->Accept (writer);
	const uint8_t* data = (const uint8_t*) buffer.GetBuffer ();
	return vector<uint8_t> (data, data + buffer.GetSize ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// JSONObject
////////////////////////////////////////////////////////////////////////////////////////////////////
shared_ptr<JSONObject> JSONObject::Create () {
	return shared_ptr<JSONObject> (new JSONObject ());
}

shared_ptr<JSONObject> JSONObject::Parse (const vector<uint8_t>& json) {
	return JSONBase::Parse<JSONObject, vector<uint8_t>> (json);
}

shared_ptr<JSONObject> JSONObject::Parse (const string& json) {
	return JSONBase::Parse<JSONObject, string> (json);
}

JSONObject::JSONObject () {
	mDocument = make_shared<rapidjson::Document> ();
	mDocument->SetObject ();
}

void JSONObject::Add (const std::string& key, const char* value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, rapidjson::StringRef (value), mDocument->GetAllocator ());
}

void JSONObject::Add (const std::string& key, const std::string& value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, rapidjson::StringRef (value.c_str ()), mDocument->GetAllocator ());
}

void JSONObject::Add (const std::string& key, bool value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, value, mDocument->GetAllocator ());
}

void JSONObject::Add (const std::string& key, int32_t value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, value, mDocument->GetAllocator ());
}

void JSONObject::Add (const std::string& key, int64_t value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, value, mDocument->GetAllocator ());
}

void JSONObject::Add (const std::string& key, uint32_t value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, value, mDocument->GetAllocator ());
}

void JSONObject::Add (const std::string& key, uint64_t value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, value, mDocument->GetAllocator ());
}

void JSONObject::Add (const std::string& key, double value) {
	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, value, mDocument->GetAllocator ());
}

void JSONObject::Add (const string& key, shared_ptr<JSONObject> value) {
	shared_ptr<JSONObject> valueImpl = static_pointer_cast<JSONObject> (value);

	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, *(valueImpl->mDocument), mDocument->GetAllocator ());
}

void JSONObject::Add (const string& key, shared_ptr<JSONArray> value) {
	shared_ptr<JSONArray> valueImpl = static_pointer_cast<JSONArray> (value);

	rapidjson::Value keyValue;
	keyValue.SetString (key.c_str (), (rapidjson::SizeType) key.length (), mDocument->GetAllocator ());
	mDocument->AddMember (keyValue, *(valueImpl->mDocument), mDocument->GetAllocator ());
}

bool JSONObject::HasString (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsString ();
}

bool JSONObject::HasBool (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsBool ();
}

bool JSONObject::HasInt32 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsInt ();
}

bool JSONObject::HasInt64 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsInt64 ();
}

bool JSONObject::HasUInt32 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsUint ();
}

bool JSONObject::HasUInt64 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsUint64 ();
}

bool JSONObject::HasDouble (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsDouble ();
}

bool JSONObject::HasObject (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsObject ();
}

bool JSONObject::HasArray (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.IsArray ();
}

string JSONObject::GetString (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? string () : string (it->value.GetString ());
}

bool JSONObject::GetBool (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.GetBool ();
}

int32_t JSONObject::GetInt32 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.GetInt ();
}

int64_t JSONObject::GetInt64 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.GetInt64 ();
}

uint32_t JSONObject::GetUInt32 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.GetUint ();
}

uint64_t JSONObject::GetUInt64 (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.GetUint64 ();
}

double JSONObject::GetDouble (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	return it == mDocument->MemberEnd () ? false : it->value.GetDouble ();
}

shared_ptr<JSONObject> JSONObject::GetObj (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	if (it == mDocument->MemberEnd ()) {
		return JSONObject::Create ();
	}

	if (!it->value.IsObject ()) {
		return JSONObject::Create ();
	}

	rapidjson::Document::AllocatorType& allocator = mDocument->GetAllocator ();
	shared_ptr<rapidjson::Document> valueDoc = make_shared<rapidjson::Document> (&allocator);
	valueDoc->Swap (it->value);
	return shared_ptr<JSONObject> (new JSONObject (valueDoc));
}

shared_ptr<JSONArray> JSONObject::GetArray (const string& key) const {
	auto it = mDocument->FindMember (rapidjson::StringRef (key.c_str ()));
	if (it == mDocument->MemberEnd ()) {
		return JSONArray::Create ();
	}

	if (!it->value.IsArray ()) {
		return JSONArray::Create ();
	}

	rapidjson::Document::AllocatorType& allocator = mDocument->GetAllocator ();
	shared_ptr<rapidjson::Document> valueDoc = make_shared<rapidjson::Document> (&allocator);
	valueDoc->Swap (it->value);
	return shared_ptr<JSONArray> (new JSONArray (valueDoc));
}

void JSONObject::IterateProperties (function<bool (const string& key, const string& value, JSONDataType type)> handleProperty) const {
	using namespace rapidjson;

	for (rapidjson::Document::ConstMemberIterator it = mDocument->MemberBegin (); it != mDocument->MemberEnd (); ++it) {
		//Convert key
		string jsonKey = it->name.GetString ();

		//Convert value
		string jsonValue;
		JSONDataType type = ValueAsString (it->value, jsonValue);
		if (type == JSONDataType::Unknown) { //Error
			return;
		}

		//Call the callback on each property
		if (!handleProperty (jsonKey, jsonValue, type)) {
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//// JSONArray
//////////////////////////////////////////////////////////////////////////////////////////////////////
shared_ptr<JSONArray> JSONArray::Create () {
	return shared_ptr<JSONArray> (new JSONArray ());
}

shared_ptr<JSONArray> JSONArray::Parse (const vector<uint8_t>& json) {
	return JSONBase::Parse<JSONArray, vector<uint8_t>> (json);
}

shared_ptr<JSONArray> JSONArray::Parse (const string& json) {
	return JSONBase::Parse<JSONArray, string> (json);
}

void JSONArray::Add (const char* value) {
	rapidjson::Value valueValue;
	valueValue.SetString (value, (rapidjson::SizeType) strlen (value), mDocument->GetAllocator ());
	mDocument->PushBack (valueValue, mDocument->GetAllocator ());
}

void JSONArray::Add (const std::string& value) {
	rapidjson::Value valueValue;
	valueValue.SetString (value.c_str (), (rapidjson::SizeType) value.length (), mDocument->GetAllocator ());
	mDocument->PushBack (valueValue, mDocument->GetAllocator ());
}

JSONArray::JSONArray () {
	mDocument = make_shared<rapidjson::Document> ();
	mDocument->SetArray ();
}

void JSONArray::Add (shared_ptr<JSONObject> value) {
	shared_ptr<JSONObject> valueImpl = static_pointer_cast<JSONObject> (value);
	mDocument->PushBack (*(valueImpl->mDocument), mDocument->GetAllocator ());
}

void JSONArray::Add (shared_ptr<JSONArray> value) {
	shared_ptr<JSONArray> valueImpl = static_pointer_cast<JSONArray> (value);
	mDocument->PushBack (*(valueImpl->mDocument), mDocument->GetAllocator ());
}

shared_ptr<JSONObject> JSONArray::GetObjectAtIndex (int32_t idx) const {
	auto& value = mDocument->operator[] (idx);
	if (!value.IsObject ()) {
		return JSONObject::Create ();
	}

	rapidjson::Document::AllocatorType& allocator = mDocument->GetAllocator ();
	shared_ptr<rapidjson::Document> valueDoc = make_shared<rapidjson::Document> (&allocator);
	valueDoc->Swap (value);
	return shared_ptr<JSONObject> (new JSONObject (valueDoc));
}

shared_ptr<JSONArray> JSONArray::GetArrayAtIndex (int32_t idx) const {
	auto& value = mDocument->operator[] (idx);
	if (!value.IsArray ()) {
		return JSONArray::Create ();
	}

	rapidjson::Document::AllocatorType& allocator = mDocument->GetAllocator ();
	shared_ptr<rapidjson::Document> valueDoc = make_shared<rapidjson::Document> (&allocator);
	valueDoc->Swap (value);
	return shared_ptr<JSONArray> (new JSONArray (valueDoc));
}

void JSONArray::IterateItems (function<bool (const string& value, int32_t idx, JSONDataType type)> handleItem) const {
	using namespace rapidjson;

	int32_t idx = 0;
	for (Document::ConstValueIterator it = mDocument->Begin (); it != mDocument->End (); ++it, ++idx) {
		string jsonValue;
		JSONDataType type = ValueAsString (*it, jsonValue);
		if (type == JSONDataType::Unknown) { //Error
			return;
		}

		//Call the callback on each property
		if (!handleItem (jsonValue, idx, type)) {
			break;
		}
	}
}
