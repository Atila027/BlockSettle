#include "ChatUsersViewModel.h"
#include "ChatClient.h"


ChatUsersViewModel::ChatUsersViewModel(QObject* parent)
   : QAbstractTableModel(parent)
{
}

QString ChatUsersViewModel::resolveUser(const QModelIndex &index) const
{
   if ((index.row() < 0) || (index.row() >= users_.size())) {
      return {};
   }
   return QString::fromStdString(users_[index.row()]);
}

int ChatUsersViewModel::columnCount(const QModelIndex &/*parent*/) const
{
   return 1;
}

int ChatUsersViewModel::rowCount(const QModelIndex &/*parent*/) const
{
   return users_.size();
}

QVariant ChatUsersViewModel::headerData(int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
   return QVariant();
}

QVariant ChatUsersViewModel::data(const QModelIndex &index, int role) const
{
   if (role == Qt::DisplayRole) {
      return resolveUser(index);
   }
   return QVariant();
}

void ChatUsersViewModel::onUsersReplace(const std::vector<std::string> &users)
{
   beginResetModel();
   users_.clear();
   users_.reserve(users.size());

   for (const auto &userId : users) {
      users_.emplace_back(std::move(userId));
   }
   endResetModel();
}

void ChatUsersViewModel::onUsersAdd(const std::vector<std::string> &users)
{
   beginInsertRows(QModelIndex(), users_.size(), users_.size() + users.size() - 1);
   for (const auto &userId : users) {
      users_.emplace_back(std::move(userId));
   }
   endInsertRows();
}

void ChatUsersViewModel::onUsersDel(const std::vector<std::string> &users)
{
   for (const auto &userId : users) {
      for (size_t i = 0; i < users_.size(); ++i) {
         if (users_[i] == userId) {
            beginRemoveRows(QModelIndex(), i, i);
            users_.erase(users_.begin() + i);
            endRemoveRows();
            break;
         }
      }
   }
}

bool ChatUsersViewModel::isUserInModel(const std::string &userId)
{
    auto iter = std::find_if(std::begin(users_), std::end(users_), [&userId](const std::string &in)
    {
        if(userId.size() != in.size())
            return false;

        std::string to_compare;
        to_compare.resize(userId.size());
        std::transform(std::begin(userId), std::end(userId), std::begin(to_compare), ::tolower);
        return (0 == to_compare.compare(in));
    });

    if(iter == std::end(users_))
        return false;

    return true;
}
