#pragma once

#include "ListResponse.h"

namespace Chat {
   ListResponse::ListResponse(ResponseType responseType, std::vector<std::string> dataList)
      : Response(responseType)
      , dataList_(dataList)
   {
   }
   
   QJsonObject ListResponse::toJson() const
   {
      QJsonObject data = Response::toJson();
   
      QJsonArray listJson;
   
      std::for_each(dataList_.begin(), dataList_.end(), [&](const std::string& userId){
         listJson << QString::fromStdString(userId);
      });
   
      data[DataKey] = listJson;
   
      return data;
   }
   
   std::vector<std::string> ListResponse::fromJSON(const std::string& jsonData)
   {
      QJsonObject data = QJsonDocument::fromJson(QString::fromStdString(jsonData).toUtf8()).object();
   
      std::vector<std::string> dataList;
      QJsonArray usersArray = data[DataKey].toArray();
      foreach(auto userId, usersArray) {
         dataList.push_back(userId.toString().toStdString());
      }
      return dataList;
   }
}
