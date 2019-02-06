#include "ChatUserListTreeWidget.h"
#include "ChatUserCategoryListView.h"

#include <QHeaderView>
#include <QtDebug>
#include <QAbstractItemView>

const QString contactsListDescription = QObject::tr("Contacts");
const QString allUsersListDescription = QObject::tr("All users");

ChatUserListTreeWidget::ChatUserListTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
   setFocusPolicy(Qt::NoFocus);
   setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
   setUniformRowHeights(false);
   setItemsExpandable(true);
   setSortingEnabled(false);
   setAnimated(true);
   setAllColumnsShowFocus(false);
   setWordWrap(false);
   setHeaderHidden(true);
   setExpandsOnDoubleClick(true);
   setColumnCount(1);
   setTextElideMode(Qt::ElideMiddle);
   setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

   _friendUsersViewModel = new ChatUsersViewModel(this);
   _nonFriendUsersViewModel = new ChatUsersViewModel(this);

   createCategories();
}

void ChatUserListTreeWidget::createCategories()
{
   QTreeWidgetItem *contactsItem = new QTreeWidgetItem(this);
   contactsItem->setText(0, contactsListDescription);
   addTopLevelItem(contactsItem);
   setItemExpanded(contactsItem, false);
   contactsItem->setFlags(Qt::NoItemFlags);

   QTreeWidgetItem *embedItem = new QTreeWidgetItem(contactsItem);
   embedItem->setFlags(Qt::ItemIsEnabled);

   ChatUserCategoryListView *listView = new ChatUserCategoryListView(this);
   listView->setViewMode(QListView::ListMode);
   listView->setModel(_friendUsersViewModel);
   listView->setObjectName(QStringLiteral("ChatUserCategoryListView"));
   setItemWidget(embedItem, 0, listView);

   connect(listView, &QAbstractItemView::clicked,
           this, &ChatUserListTreeWidget::onUserListItemClicked);

   QTreeWidgetItem *allUsers = new QTreeWidgetItem(this);
   allUsers->setText(0, allUsersListDescription);
   addTopLevelItem(allUsers);
   setItemExpanded(allUsers, false);
   allUsers->setFlags(Qt::NoItemFlags);

   QTreeWidgetItem *embedAllUsers = new QTreeWidgetItem(allUsers);
   embedAllUsers->setFlags(Qt::ItemIsEnabled);

   ChatUserCategoryListView *listViewAllUsers = new ChatUserCategoryListView(this);
   listViewAllUsers->setViewMode(QListView::ListMode);
   listViewAllUsers->setModel(_nonFriendUsersViewModel);
   listViewAllUsers->setObjectName(QStringLiteral("ChatUserCategoryListView"));
   setItemWidget(embedAllUsers, 0, listViewAllUsers);

   connect(listViewAllUsers, &QAbstractItemView::clicked,
           this, &ChatUserListTreeWidget::onUserListItemClicked);

   adjustListViewSize();
}

void ChatUserListTreeWidget::onChatUserDataListChanged(const TChatUserDataListPtr &chatUserDataList)
{
   TChatUserDataListPtr friendList;
   TChatUserDataListPtr nonFriendList;

   std::for_each(std::begin(chatUserDataList), std::end(chatUserDataList), [&friendList, &nonFriendList](const TChatUserDataPtr &userDataPtr)
   {
      if (userDataPtr->userState() == ChatUserData::Unknown)
      {
         nonFriendList.push_back(userDataPtr);
      }
      else
      {
         friendList.push_back(userDataPtr);
      }
   });

   _friendUsersViewModel->onUserDataListChanged(friendList);
   _nonFriendUsersViewModel->onUserDataListChanged(nonFriendList);

   adjustListViewSize();
}

void ChatUserListTreeWidget::adjustListViewSize()
{
   int idx = topLevelItemCount();
   for (int i = 0; i < idx; i++)
   {
      QTreeWidgetItem *item = topLevelItem(i);
      QTreeWidgetItem *embedItem = item->child(0);
      if (embedItem == nullptr)
         continue;

      ChatUserCategoryListView *listWidget = qobject_cast<ChatUserCategoryListView*>(itemWidget(embedItem, 0));
      listWidget->doItemsLayout();
      const int height = qMax(listWidget->contentsSize().height(), 0);
      listWidget->setFixedHeight(height);
      if(listWidget->model()->rowCount())
      {
         item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
         setItemExpanded(item, true);
         item->setFlags(Qt::ItemIsEnabled);
      }
      else
      {
         item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicator);
         setItemExpanded(item, false);
         item->setFlags(Qt::NoItemFlags);
      }
   }
}

ChatUserCategoryListView *ChatUserListTreeWidget::listViewAt(int idx) const
{
   ChatUserCategoryListView *listWidget = nullptr;
   if (QTreeWidgetItem *item = topLevelItem(idx))
      if (QTreeWidgetItem *embedItem = item->child(0))
         listWidget = qobject_cast<ChatUserCategoryListView*>(itemWidget(embedItem, 0));
   Q_ASSERT(listWidget);

   return listWidget;
}

void ChatUserListTreeWidget::onUserListItemClicked(const QModelIndex &index)
{
   ChatUserCategoryListView *listView = qobject_cast<ChatUserCategoryListView*>(sender());

   if (!listView)
   {
      return;
   }

   ChatUsersViewModel *model = qobject_cast<ChatUsersViewModel*>(listView->model());
   if (!model)
   {
      return;
   }

   QString userId = model->data(index, Qt::DisplayRole).toString();
   emit userClicked(userId);
}
