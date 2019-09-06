#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "libpg_query/pg_query.h"
#include "parser/parsenodes.h"
#include "parser/statements.h"

namespace terrier {
namespace parser {

/**
 * ParseResult is the parser's output to the binder. It allows you to obtain non-owning managed pointers to the
 * statements and expressions that were generated during the parse. If you need to take ownership, you can do that
 * too, but then the parse result's copy is invalidated.
 */
class ParseResult {
 public:
  /**
   * Adds a statement to this parse result.
   */
  void AddStatement(std::unique_ptr<SQLStatement> statement) { statements_.emplace_back(std::move(statement)); }

  /**
   * Adds an expression to this parse result.
   */
  void AddExpression(std::unique_ptr<AbstractExpression> expression) {
    expressions_.emplace_back(std::move(expression));
  }

  /**
   * @return non-owning list of all the statements contained in this parse result
   */
  std::vector<common::ManagedPointer<SQLStatement>> GetStatements() {
    std::vector<common::ManagedPointer<SQLStatement>> statements;
    statements.reserve(statements_.size());
    for (const auto &statement : statements_) {
      statements.emplace_back(common::ManagedPointer(statement));
    }
    return statements;
  }

  /**
   * @return the statement at a particular index
   */
  common::ManagedPointer<SQLStatement> GetStatement(size_t idx) { return common::ManagedPointer(statements_[idx]); }

  /**
   * @return non-owning list of all the expressions contained in this parse result
   */
  std::vector<common::ManagedPointer<AbstractExpression>> GetExpressions() {
    std::vector<common::ManagedPointer<AbstractExpression>> expressions;
    expressions.reserve(expressions_.size());
    for (const auto &statement : expressions_) {
      expressions.emplace_back(common::ManagedPointer(statement));
    }
    return expressions;
  }

  /**
   * @return the expression at a particular index
   */
  common::ManagedPointer<AbstractExpression> GetExpression(size_t idx) {
    return common::ManagedPointer(expressions_[idx]);
  }

  /**
   * Returns ownership of the statements in this parse result.
   * @return moved statements
   */
  std::vector<std::unique_ptr<SQLStatement>> &&TakeStatementsOwnership() { return std::move(statements_); }

  /**
   * Returns ownership of the expressions in this parse result.
   * @return moved expressions
   */
  std::vector<std::unique_ptr<AbstractExpression>> &&TakeExpressionsOwnership() { return std::move(expressions_); }

 private:
  std::vector<std::unique_ptr<SQLStatement>> statements_;
  std::vector<std::unique_ptr<AbstractExpression>> expressions_;
};

/**
 * PostgresParser obtains and transforms the Postgres parse tree into our Terrier parse tree.
 * In the future, we definitely want to replace this with our own parser.
 */
class PostgresParser {
  /*
   * To modify this file, examine:
   *    List and ListCell in pg_list.h,
   *    Postgres types in nodes.h.
   *
   * To add new Statement support, find the parsenode in:
   *    third_party/libpg_query/src/postgres/include/nodes/parsenodes.h,
   *    third_party/libpg_query/src/postgres/include/nodes/primnodes.h,
   * then copy to src/include/parser/parsenodes.h and add the corresponding helper function.
   */

 public:
  PostgresParser();
  ~PostgresParser();

  /**
   * Builds the parse tree for the given query string.
   * @param query_string query string to be parsed
   * @return unique pointer to parse tree
   */
  std::vector<std::unique_ptr<SQLStatement>> BuildParseTree(const std::string &query_string);

 private:
  static FKConstrActionType CharToActionType(const char &type) {
    switch (type) {
      case 'a':
        return FKConstrActionType::NOACTION;
      case 'r':
        return FKConstrActionType::RESTRICT_;
      case 'c':
        return FKConstrActionType::CASCADE;
      case 'n':
        return FKConstrActionType::SETNULL;
      case 'd':
        return FKConstrActionType::SETDEFAULT;
      default:
        return FKConstrActionType::NOACTION;
    }
  }

  static FKConstrMatchType CharToMatchType(const char &type) {
    switch (type) {
      case 'f':
        return FKConstrMatchType::FULL;
      case 'p':
        return FKConstrMatchType::PARTIAL;
      case 's':
        return FKConstrMatchType::SIMPLE;
      default:
        return FKConstrMatchType::SIMPLE;
    }
  }

  static bool IsAggregateFunction(const std::string &fun_name) {
    return (fun_name == "min" || fun_name == "max" || fun_name == "count" || fun_name == "avg" || fun_name == "sum");
  }

  /**
   * Transforms the entire parsed nodes list into a corresponding SQLStatementList.
   * @param[in,out] parse_result the current parse result, which will be updated
   * @param root list of parsed nodes
   */
  static std::vector<std::unique_ptr<SQLStatement>> ListTransform(List *root);

  /**
   * Transforms a single node in the parse list into a terrier SQLStatement object.
   * @param[in,out] parse_result the current parse result, which will be updated
   * @param node parsed node
   * @return SQLStatement corresponding to the parsed node
   */
  static std::unique_ptr<SQLStatement> NodeTransform(Node *node);

  static std::unique_ptr<AbstractExpression> ExprTransform(ParseResult *parse_result, Node *node, char *alias);
  static ExpressionType StringToExpressionType(const std::string &parser_str);
  static std::unique_ptr<AbstractExpression> AExprTransform(ParseResult *parse_result, A_Expr *root);
  static std::unique_ptr<AbstractExpression> BoolExprTransform(ParseResult *parse_result, BoolExpr *root);
  static std::unique_ptr<AbstractExpression> CaseExprTransform(ParseResult *parse_result, CaseExpr *root);
  static std::unique_ptr<AbstractExpression> ColumnRefTransform(ParseResult *parse_result, ColumnRef *root,
                                                                char *alias);
  static std::unique_ptr<AbstractExpression> ConstTransform(ParseResult *parse_result, A_Const *root);
  static std::unique_ptr<AbstractExpression> FuncCallTransform(ParseResult *parse_result, FuncCall *root);
  static std::unique_ptr<AbstractExpression> NullTestTransform(ParseResult *parse_result, NullTest *root);
  static std::unique_ptr<AbstractExpression> ParamRefTransform(ParseResult *parse_result, ParamRef *root);
  static std::unique_ptr<AbstractExpression> SubqueryExprTransform(ParseResult *parse_result, SubLink *node);
  static std::unique_ptr<AbstractExpression> TypeCastTransform(ParseResult *parse_result, TypeCast *root);
  static std::unique_ptr<AbstractExpression> ValueTransform(ParseResult *parse_result, value val);

  // SELECT statements
  static std::unique_ptr<SelectStatement> SelectTransform(ParseResult *parse_result, SelectStmt *root);
  // SELECT helpers
  static std::vector<std::unique_ptr<AbstractExpression>> TargetTransform(List *root);
  static std::unique_ptr<TableRef> FromTransform(SelectStmt *select_root);
  static std::unique_ptr<GroupByDescription> GroupByTransform(List *group, Node *having_node);
  static std::unique_ptr<OrderByDescription> OrderByTransform(List *order);
  static std::unique_ptr<AbstractExpression> WhereTransform(Node *root);

  // FromTransform helpers
  static std::unique_ptr<JoinDefinition> JoinTransform(ParseResult *parse_result, JoinExpr *root);
  static std::string AliasTransform(Alias *root);
  static std::unique_ptr<TableRef> RangeVarTransform(ParseResult *parse_result, RangeVar *root);
  static std::unique_ptr<TableRef> RangeSubselectTransform(ParseResult *parse_result, RangeSubselect *root);

  // COPY statements
  static std::unique_ptr<CopyStatement> CopyTransform(ParseResult *parse_result, CopyStmt *root);

  // CREATE statements
  static std::unique_ptr<SQLStatement> CreateTransform(ParseResult *parse_result, CreateStmt *root);
  static std::unique_ptr<SQLStatement> CreateDatabaseTransform(ParseResult *parse_result, CreateDatabaseStmt *root);
  static std::unique_ptr<SQLStatement> CreateFunctionTransform(ParseResult *parse_result, CreateFunctionStmt *root);
  static std::unique_ptr<SQLStatement> CreateIndexTransform(ParseResult *parse_result, IndexStmt *root);
  static std::unique_ptr<SQLStatement> CreateSchemaTransform(ParseResult *parse_result, CreateSchemaStmt *root);
  static std::unique_ptr<SQLStatement> CreateTriggerTransform(ParseResult *parse_result, CreateTrigStmt *root);
  static std::unique_ptr<SQLStatement> CreateViewTransform(ParseResult *parse_result, ViewStmt *root);

  // CREATE helpers
  using ColumnDefTransResult = struct {
    std::unique_ptr<ColumnDefinition> col;
    std::vector<std::unique_ptr<ColumnDefinition>> fks;
  };
  static ColumnDefTransResult ColumnDefTransform(ParseResult *parse_result, ColumnDef *root);

  // CREATE FUNCTION helpers
  static std::unique_ptr<FuncParameter> FunctionParameterTransform(ParseResult *parse_result, FunctionParameter *root);
  static std::unique_ptr<ReturnType> ReturnTypeTransform(ParseResult *parse_result, TypeName *root);

  // CREATE TRIGGER helpers
  static std::unique_ptr<AbstractExpression> WhenTransform(ParseResult *parse_result, Node *root);

  // DELETE statements
  static std::unique_ptr<DeleteStatement> DeleteTransform(ParseResult *parse_result, DeleteStmt *root);

  // DROP statements
  static std::unique_ptr<DropStatement> DropTransform(ParseResult *parse_result, DropStmt *root);
  static std::unique_ptr<DropStatement> DropDatabaseTransform(ParseResult *parse_result, DropDatabaseStmt *root);
  static std::unique_ptr<DropStatement> DropIndexTransform(ParseResult *parse_result, DropStmt *root);
  static std::unique_ptr<DropStatement> DropSchemaTransform(ParseResult *parse_result, DropStmt *root);
  static std::unique_ptr<DropStatement> DropTableTransform(ParseResult *parse_result, DropStmt *root);
  static std::unique_ptr<DropStatement> DropTriggerTransform(ParseResult *parse_result, DropStmt *root);

  // EXECUTE statements
  static std::unique_ptr<ExecuteStatement> ExecuteTransform(ParseResult *parse_result, ExecuteStmt *root);

  // EXECUTE helpers
  static std::vector<std::unique_ptr<AbstractExpression>> ParamListTransform(List *root);

  // EXPLAIN statements
  static std::unique_ptr<ExplainStatement> ExplainTransform(ParseResult *parse_result, ExplainStmt *root);

  // INSERT statements
  static std::unique_ptr<InsertStatement> InsertTransform(ParseResult *parse_result, InsertStmt *root);

  // INSERT helpers
  static std::unique_ptr<std::vector<std::string>> ColumnNameTransform(List *root);
  static std::unique_ptr<std::vector<std::vector<std::unique_ptr<AbstractExpression>>>> ValueListsTransform(List *root);

  // PREPARE statements
  static std::unique_ptr<PrepareStatement> PrepareTransform(ParseResult *parse_result, PrepareStmt *root);

  static std::unique_ptr<DeleteStatement> TruncateTransform(ParseResult *parse_result, TruncateStmt *truncate_stmt);

  /**
   * Converts a TRANSACTION statement from postgres parser form to internal form
   *
   * @param transaction_stmt from the postgres parser
   * @return converted to parser::TransactionStatement
   */
  static std::unique_ptr<TransactionStatement> TransactionTransform(TransactionStmt *transaction_stmt);

  // VACUUM statements as ANALYZE statements
  static std::unique_ptr<AnalyzeStatement> VacuumTransform(ParseResult *parse_result, VacuumStmt *root);

  // VARIABLE SET statements
  static std::unique_ptr<VariableSetStatement> VariableSetTransform(ParseResult *parse_result, VariableSetStmt *root);

  /**
   * Converts the target of an update clause, i.e. one or more column = expression
   * statements, from postgres parser form to internal form
   * @param root list of targets
   * @return vector of update clauses
   */
  static std::vector<std::unique_ptr<parser::UpdateClause>> UpdateTargetTransform(List *root);

  /**
   * Converts an UPDATE statement from postgres parser form to our internal form.
   * @param update_stmt from the postgres parser
   * @return converted to a parser::UpdateStatement
   *
   * TODO: Does not support:
   * - with clause
   * - from clause
   * - returning a list
   */
  static std::unique_ptr<UpdateStatement> UpdateTransform(ParseResult *parse_result, UpdateStmt *update_stmt);
};

}  // namespace parser
}  // namespace terrier
