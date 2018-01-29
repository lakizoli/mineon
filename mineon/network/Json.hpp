#pragma once

#include <rapidjson/document.h>

enum class JSONDataType {
	Unknown,

	String,
	Bool,
	Int32,
	Int64,
	UInt32,
	UInt64,
	Double,
	Object,
	Array
};

class JSONBase {
	friend class JSONObject;
	friend class JSONArray;

protected:
	std::shared_ptr<rapidjson::Document> mDocument;

	explicit JSONBase (std::shared_ptr<rapidjson::Document> document) : mDocument (document) {}
	JSONBase () {}

	JSONBase (const JSONBase& src) = delete;
	JSONBase (JSONBase&& src) = delete;

	JSONBase& operator= (const JSONBase& src) = delete;
	JSONBase& operator= (JSONBase&& src) = delete;

	template<class ResultT, class ContainerT>
	static std::shared_ptr<ResultT> Parse (const ContainerT& jsonUTF8);

	template<typename Encoding, typename Allocator = rapidjson::MemoryPoolAllocator<>>
	JSONDataType ValueAsString (const rapidjson::GenericValue<Encoding, Allocator>& value, std::string& jsonValue) const;

	std::string ToString (bool prettyPrint = false) const;
	std::vector<uint8_t> ToVector () const;

public:
	virtual ~JSONBase () {}
};

class JSONObject : public JSONBase {
	friend class JSONArray;
	friend class JSONBase;

	explicit JSONObject (std::shared_ptr<rapidjson::Document> document) : JSONBase (document) {}
	JSONObject ();

public:
	static std::shared_ptr<JSONObject> Create ();

	static std::shared_ptr<JSONObject> Parse (const std::vector<uint8_t>& json);
	static std::shared_ptr<JSONObject> Parse (const std::string& json);

	virtual ~JSONObject () {}

	JSONObject (const JSONObject& src) = delete;
	JSONObject (JSONObject&& src) = delete;

	JSONObject& operator= (const JSONObject& src) = delete;
	JSONObject& operator= (JSONObject&& src) = delete;

public:
	void AddNull (const std::string& key);

	void Add (const std::string& key, const char* value);
	void Add (const std::string& key, const std::string& value);
	void Add (const std::string& key, bool value);
	void Add (const std::string& key, int32_t value);
	void Add (const std::string& key, int64_t value);
	void Add (const std::string& key, uint32_t value);
	void Add (const std::string& key, uint64_t value);
	void Add (const std::string& key, double value);
	void Add (const std::string& key, std::shared_ptr<JSONObject> value);
	void Add (const std::string& key, std::shared_ptr<JSONArray> value);

	bool IsEmpty () const { return mDocument->ObjectEmpty (); }

	bool HasString (const std::string& key) const;
	bool HasBool (const std::string& key) const;
	bool HasInt32 (const std::string& key) const;
	bool HasInt64 (const std::string& key) const;
	bool HasUInt32 (const std::string& key) const;
	bool HasUInt64 (const std::string& key) const;
	bool HasDouble (const std::string& key) const;
	bool HasObject (const std::string& key) const;
	bool HasArray (const std::string& key) const;

	std::string GetString (const std::string& key) const;
	bool GetBool (const std::string& key) const;
	int32_t GetInt32 (const std::string& key) const;
	int64_t GetInt64 (const std::string& key) const;
	uint32_t GetUInt32 (const std::string& key) const;
	uint64_t GetUInt64 (const std::string& key) const;
	double GetDouble (const std::string& key) const;
	std::shared_ptr<JSONObject> GetObj (const std::string& key) const;
	std::shared_ptr<JSONArray> GetArray (const std::string& key) const;

	std::string ToString (bool prettyPrint = false) const { return JSONBase::ToString (prettyPrint); }
	std::vector<uint8_t> ToVector () const { return JSONBase::ToVector (); }

	void IterateProperties (std::function<bool (const std::string& key, const std::string& value, JSONDataType type)> handleProperty) const;
};

class JSONArray : public JSONBase {
	friend class JSONObject;
	friend class JSONBase;

	explicit JSONArray (std::shared_ptr<rapidjson::Document> document) : JSONBase (document) {}
	JSONArray ();

public:
	static std::shared_ptr<JSONArray> Create ();

	static std::shared_ptr<JSONArray> Parse (const std::vector<uint8_t>& json);
	static std::shared_ptr<JSONArray> Parse (const std::string& json);

	virtual ~JSONArray () {}

	JSONArray (const JSONArray& src) = delete;
	JSONArray (JSONArray&& src) = delete;

	JSONArray& operator = (const JSONArray& src) = delete;
	JSONArray& operator = (JSONArray&& src) = delete;

public:
	void Add (const char* value);
	void Add (const std::string& value);
	void Add (bool value) { mDocument->PushBack (value, mDocument->GetAllocator ()); }
	void Add (int32_t value) { mDocument->PushBack (value, mDocument->GetAllocator ()); }
	void Add (int64_t value) { mDocument->PushBack (value, mDocument->GetAllocator ()); }
	void Add (uint32_t value) { mDocument->PushBack (value, mDocument->GetAllocator ()); }
	void Add (uint64_t value) { mDocument->PushBack (value, mDocument->GetAllocator ()); }
	void Add (double value) { mDocument->PushBack (value, mDocument->GetAllocator ()); }
	void Add (std::shared_ptr<JSONObject> value);
	void Add (std::shared_ptr<JSONArray> value);

	int32_t GetCount () const { return mDocument->Size (); }

	bool HasStringAtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsString (); }
	bool HasBoolAtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsBool (); }
	bool HasInt32AtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsInt (); }
	bool HasInt64AtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsInt64 (); }
	bool HasUInt32AtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsUint (); }
	bool HasUInt64AtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsUint64 (); }
	bool HasDoubleAtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsDouble (); }
	bool HasObjectAtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsObject (); }
	bool HasArrayAtIndex (int32_t idx) const { return mDocument->operator[] (idx).IsArray (); }

	std::string GetStringAtIndex (int32_t idx) const { return mDocument->operator[] (idx).GetString (); }
	bool GetBoolAtIndex (int32_t idx) const { return mDocument->operator[] (idx).GetBool (); }
	int32_t GetInt32AtIndex (int32_t idx) const { return mDocument->operator[] (idx).GetInt (); }
	int64_t GetInt64AtIndex (int32_t idx) const { return mDocument->operator[] (idx).GetInt64 (); }
	uint32_t GetUInt32AtIndex (int32_t idx) const { return mDocument->operator[] (idx).GetUint (); }
	uint64_t GetUInt64AtIndex (int32_t idx) const { return mDocument->operator[] (idx).GetUint64 (); }
	double GetDoubleAtIndex (int32_t idx) const { return mDocument->operator[] (idx).GetDouble (); }
	std::shared_ptr<JSONObject> GetObjectAtIndex (int32_t idx) const;
	std::shared_ptr<JSONArray> GetArrayAtIndex (int32_t idx) const;

	std::string ToString (bool prettyPrint = false) const { return JSONBase::ToString (prettyPrint); }
	std::vector<uint8_t> ToVector () const { return JSONBase::ToVector (); }

	void IterateItems (std::function<bool (const std::string& value, int32_t idx, JSONDataType type)> handleItem) const;
};
