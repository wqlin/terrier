#include "parser/table_ref.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "parser/select_statement.h"

namespace terrier::parser {

/**
 * @return JoinDefinition serialized to json
 */
nlohmann::json JoinDefinition::ToJson() const {
  nlohmann::json j;
  j["type"] = type_;
  // TODO(WAN)
  //  j["left"] = left_;
  //  j["right"] = right_;
  //  j["condition"] = condition_;
  return j;
}

/**
 * @param j json to deserialize
 */
std::vector<std::unique_ptr<AbstractExpression>> JoinDefinition::FromJson(const nlohmann::json &j) {
  std::vector<std::unique_ptr<AbstractExpression>> exprs;
  // Deserialize type
  type_ = j.at("type").get<JoinType>();

  // Deserialize left
  if (!j.at("left").is_null()) {
    left_ = std::make_unique<TableRef>();
    left_->FromJson(j.at("left"));
  }

  // Deserialize right
  if (!j.at("right").is_null()) {
    right_ = std::make_unique<TableRef>();
    right_->FromJson(j.at("right"));
  }

  // Deserialize condition
  if (!j.at("condition").is_null()) {
    auto deserialized = DeserializeExpression(j.at("condition"));
    condition_ = common::ManagedPointer(deserialized.result_);
    exprs.emplace_back(std::move(deserialized.result_));
    exprs.insert(exprs.end(), std::make_move_iterator(deserialized.non_owned_exprs_.begin()),
                 std::make_move_iterator(deserialized.non_owned_exprs_.end()));
  }

  return exprs;
}

std::unique_ptr<JoinDefinition> JoinDefinition::Copy() {
  return std::make_unique<JoinDefinition>(type_, left_->Copy(), right_->Copy(), condition_);
}

nlohmann::json TableRef::ToJson() const {
  nlohmann::json j;
  j["type"] = type_;
  j["alias"] = alias_;
  //  j["table_info"] = table_info_;
  //  j["select"] = select_;
  //  j["list"] = list_;
  //  j["join"] = join_;
  return j;
}

std::vector<std::unique_ptr<AbstractExpression>> TableRef::FromJson(const nlohmann::json &j) {
  std::vector<std::unique_ptr<AbstractExpression>> exprs;
  // Deserialize type
  type_ = j.at("type").get<TableReferenceType>();

  // Deserialize alias
  alias_ = j.at("alias").get<std::string>();

  // Deserialize table info
  if (!j.at("table_info").is_null()) {
    table_info_ = std::make_unique<TableInfo>();
    table_info_->FromJson(j.at("table_info"));
  }

  // Deserialize select
  if (!j.at("select").is_null()) {
    select_ = std::make_unique<parser::SelectStatement>();
    select_->FromJson(j.at("select"));
  }

  // Deserialize list
  auto list_jsons = j.at("list").get<std::vector<nlohmann::json>>();
  for (const auto &list_json : list_jsons) {
    auto ref = std::make_unique<TableRef>();
    ref->FromJson(list_json);
    list_.emplace_back(std::move(ref));
  }

  // Deserialize join
  if (!j.at("join").is_null()) {
    join_ = std::make_unique<JoinDefinition>();
    join_->FromJson(j.at("join"));
  }

  return exprs;
}

std::unique_ptr<TableRef> TableRef::Copy() {
  auto table_ref = std::make_unique<TableRef>();

  table_ref->type_ = type_;
  table_ref->alias_ = alias_;
  table_ref->table_info_ = table_info_ == nullptr ? nullptr : table_info_->Copy();
  table_ref->select_ = select_ == nullptr ? nullptr : select_->Copy();
  table_ref->join_ = join_ == nullptr ? nullptr : join_->Copy();

  table_ref->list_.reserve(list_.size());
  for (const auto &item : list_) {
    table_ref->list_.emplace_back(item->Copy());
  }
  return table_ref;
}
}  // namespace terrier::parser
