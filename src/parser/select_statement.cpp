#include "parser/select_statement.h"

#include <memory>
#include <utility>
#include <vector>

namespace terrier::parser {

nlohmann::json SelectStatement::ToJson() const {
  nlohmann::json j = SQLStatement::ToJson();
  //  j["select"] = select_;
  //  j["select_distinct"] = select_distinct_;
  // TODO(WAN)  j["from"] = from_;
  //  j["where"] = where_;
  //  j["group_by"] = group_by_;
  //  j["order_by"] = order_by_;
  //  j["limit"] = limit_;
  //  j["union_select"] = union_select_;
  return j;
}

std::vector<std::unique_ptr<AbstractExpression>> SelectStatement::FromJson(const nlohmann::json &j) {
  std::vector<std::unique_ptr<AbstractExpression>> exprs;

  auto e1 = SQLStatement::FromJson(j);
  exprs.insert(exprs.end(), std::make_move_iterator(e1.begin()), std::make_move_iterator(e1.end()));

  // Deserialize select
  auto select_expressions = j.at("select").get<std::vector<nlohmann::json>>();
  for (const auto &expr : select_expressions) {
    auto deserialized = DeserializeExpression(expr);
    select_.emplace_back(common::ManagedPointer(deserialized.result_));
    exprs.emplace_back(std::move(deserialized.result_));
    exprs.insert(exprs.end(), std::make_move_iterator(deserialized.non_owned_exprs_.begin()),
                 std::make_move_iterator(deserialized.non_owned_exprs_.end()));
  }
  // Deserialize select distinct
  select_distinct_ = j.at("select_distinct").get<bool>();

  // Deserialize from
  if (!j.at("from").is_null()) {
    from_ = std::make_unique<TableRef>();
    from_->FromJson(j.at("from"));
  }

  // Deserialize where
  if (!j.at("where").is_null()) {
    auto deserialized = DeserializeExpression(j.at("where"));
    where_ = common::ManagedPointer(deserialized.result_);
    exprs.emplace_back(std::move(deserialized.result_));
    exprs.insert(exprs.end(), std::make_move_iterator(deserialized.non_owned_exprs_.begin()),
                 std::make_move_iterator(deserialized.non_owned_exprs_.end()));
  }

  // Deserialize group by
  if (!j.at("group_by").is_null()) {
    group_by_ = std::make_unique<GroupByDescription>();
    group_by_->FromJson(j.at("group_by"));
  }

  // Deserialize order by
  if (!j.at("order_by").is_null()) {
    order_by_ = std::make_unique<OrderByDescription>();
    order_by_->FromJson(j.at("order_by"));
  }

  // Deserialize limit
  if (!j.at("limit").is_null()) {
    limit_ = std::make_unique<LimitDescription>();
    limit_->FromJson(j.at("limit"));
  }

  // Deserialize select
  if (!j.at("union_select").is_null()) {
    union_select_ = std::make_unique<parser::SelectStatement>();
    union_select_->FromJson(j.at("union_select"));
  }
  return select;
}

}  // namespace terrier::parser
