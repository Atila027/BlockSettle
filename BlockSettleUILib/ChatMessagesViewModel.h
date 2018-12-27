#ifndef __CHAT_MESSAGES_VIEW_MODEL_H__
#define __CHAT_MESSAGES_VIEW_MODEL_H__


#include <QAbstractTableModel>
#include <QMap>
#include <QVector>
#include <QDateTime>

#include <memory>


typedef std::vector<std::pair<QDateTime, QString>> MessagesHistory;


class ChatMessagesViewModel : public QAbstractTableModel
{
   Q_OBJECT

public:

   ChatMessagesViewModel(QObject* parent = nullptr);
   ~ChatMessagesViewModel() noexcept override = default;

   ChatMessagesViewModel(const ChatMessagesViewModel&) = delete;
   ChatMessagesViewModel& operator = (const ChatMessagesViewModel&) = delete;

   ChatMessagesViewModel(ChatMessagesViewModel&&) = delete;
   ChatMessagesViewModel& operator = (ChatMessagesViewModel&&) = delete;

public:
   int columnCount(const QModelIndex &parent = QModelIndex()) const override;
   int rowCount(const QModelIndex &parent = QModelIndex()) const override;

   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
   QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
   void ensureChatId(const QString& chatId);
   QString prependMessage(const QString& messageText, const QString& senderId = QString());

public slots:
   void onSwitchToChat(const QString& chatId);
   void onMessagesUpdate(const std::vector<std::string>& messages);
   void onSingleMessageUpdate(const QDateTime&, const QString& messageText);

private:
   QMap<QString, MessagesHistory> messages_;
   QString currentChatId_;
};

#endif
