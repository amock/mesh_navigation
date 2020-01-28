#include "mesh_client/mesh_client.h"

#include <curl/curl.h>

namespace mesh_client
{
MeshClient::MeshClient(const std::string& server_url, const std::string& server_username,
                       const std::string& server_password)
  : server_url_(server_url), server_username_(server_username), server_password_(server_password)
{
}

void MeshClient::setBoundingBox(float min_x, float min_y, float min_z, const float max_x, const float max_y,
                                const float max_z)
{
  bounding_box_[0] = min_x;
  bounding_box_[1] = min_y;
  bounding_box_[2] = min_z;
  bounding_box_[3] = max_x;
  bounding_box_[4] = max_y;
  bounding_box_[5] = max_z;
}

void MeshClient::addFilter(std::string channel, float min_value, float max_value)
{
  mesh_filters_[channel] = std::make_pair(min_value, max_value);
}

std::string MeshClient::buildJson(const std::string& attribute_name)
{
  Json::Value attr;
  attr["name"] = attribute_name;

  if (mesh_filters_.size() > 0)
  {
    Json::Value filters;
    for (auto& mesh_filter : mesh_filters_)
    {
      Json::Value filter;

      filter["attribute_name"] = mesh_filter.first;
      filter["min_val"] = mesh_filter.second.first;
      filter["max_val"] = mesh_filter.second.second;

      filters.append(filter);
    }
    attr["filters"] = filters;
  }

  Json::Value json_bb;
  json_bb["x_min"] = bounding_box_[0];
  json_bb["y_min"] = bounding_box_[1];
  json_bb["z_min"] = bounding_box_[2];
  json_bb["x_max"] = bounding_box_[3];
  json_bb["y_max"] = bounding_box_[4];
  json_bb["z_max"] = bounding_box_[5];

  Json::Value request;
  request["boundingbox"] = json_bb;
  request["attribute"] = attr;
  Json::FastWriter fast_writer;
  return fast_writer.write(request);
}

bool parseByteDataString(const std::string& string, char& type, unsigned long& size, unsigned long& width, char*& data)
{
  if (string.length() < 10)
    return false;
  // parse body
  const char* body = string.c_str();
  type = body[0];                                             // one byte
  size = *reinterpret_cast<const unsigned long*>(body + 1);   // eight bytes
  width = *reinterpret_cast<const unsigned long*>(body + 9);  // eight bytes
  data = reinterpret_cast<char*>(const_cast<char*>(body + 17));
  return true;
}

lvr2::FloatChannelOptional MeshClient::getVertices()
{
  if (float_channels.find("vertices") != float_channels.end())
  {
    return float_channels["vertices"];
  }

  unique_ptr<std::string> str = requestChannel("vertices");

  if (str && str->size() > 17)
  {
    char type;
    unsigned long size, width;
    char* data;

    bool ok = parseByteDataString(*str, type, size, width, data) && type == Type::FLOAT;

    std::cout << "ok: " << ok << " type: " << (((int)type) & 0xFF) << " size: " << size << " width: " << width
              << std::endl;

    if (ok)
    {
      float* float_data = reinterpret_cast<float*>(data);
      auto channel = lvr2::FloatChannel(size, width);
      memcpy(channel.dataPtr().get(), float_data, size * width * sizeof(float));

      return channel;
    }
  }
  std::cout << "failed to load vertices!" << std::endl;
  return lvr2::FloatChannelOptional();
}

lvr2::IndexChannelOptional MeshClient::getIndices()
{
  if (index_channels.find("face_indices") != index_channels.end())
  {
    return index_channels["face_indices"];
  }

  unique_ptr<std::string> str = requestChannel("face_indices");

  if (str && str->size() > 17)
  {
    char type;
    unsigned long size, width;
    char* data;

    bool ok = parseByteDataString(*str, type, size, width, data) && type == Type::UINT;
    std::cout << "ok: " << ok << " type: " << (((int)type) & 0xFF) << " size: " << size << " width: " << width
              << std::endl;

    if (ok)
    {
      unsigned int* index_data = reinterpret_cast<unsigned int*>(data);
      auto channel = lvr2::IndexChannel(size, width);
      memcpy(channel.dataPtr().get(), index_data, size * width * sizeof(lvr2::Index));

      return channel;
    }
  }
  std::cout << "failed to load face_indices!" << std::endl;
  return lvr2::IndexChannelOptional();
}

bool MeshClient::addVertices(const lvr2::FloatChannel& channel)
{
  float_channels["vertices"] = channel;
  return true;
}

bool MeshClient::addIndices(const lvr2::IndexChannel& channel)
{
  index_channels["face_indices"] = channel;
  return true;
}

bool MeshClient::getChannel(const std::string group, const std::string name, lvr2::FloatChannelOptional& channel)
{
  std::cout << "channel " << name << std::endl;
  if (float_channels.find(name) != float_channels.end())
  {
    channel = float_channels[name];
    return true;
  }

  unique_ptr<std::string> str = requestChannel(name);

  if (str && str->size() > 17)
  {
    char type;
    unsigned long size, width;
    char* data;

    if (parseByteDataString(*str, type, size, width, data) && type == Type::FLOAT)
    {
      float* float_data = reinterpret_cast<float*>(data);
      channel = lvr2::FloatChannel(size, width);
      memcpy(channel.get().dataPtr().get(), float_data, size * width * sizeof(float));
      return true;
    }
  }
  std::cout << "failed to load " << name << "!" << std::endl;
  return false;
}

bool MeshClient::getChannel(const std::string group, const std::string name, lvr2::IndexChannelOptional& channel)
{
  if (index_channels.find(name) != index_channels.end())
  {
    channel = index_channels[name];
    return true;
  }

  unique_ptr<std::string> str = requestChannel(name);

  if (str && str->size() > 17)
  {
    char type;
    unsigned long size, width;
    char* data;

    if (!parseByteDataString(*str, type, size, width, data) && type == Type::UINT)
    {
      unsigned int* index_data = reinterpret_cast<unsigned int*>(data);
      channel = lvr2::IndexChannel(size, width);
      memcpy(channel.get().dataPtr().get(), index_data, size * width * sizeof(lvr2::Index));
      return true;
    }
  }
  std::cout << "failed to load " << name << "!" << std::endl;
  return false;
}

bool MeshClient::getChannel(const std::string group, const std::string name, lvr2::UCharChannelOptional& channel)
{
  if (uchar_channels.find(name) != uchar_channels.end())
  {
    channel = uchar_channels[name];
    return true;
  }

  unique_ptr<std::string> str = requestChannel(name);

  if (str && str->size() > 17)
  {
    char type;
    unsigned long size, width;
    char* data;
    {
      unsigned char* uchar_data = reinterpret_cast<unsigned char*>(data);
      channel = lvr2::UCharChannel(size, width);
      memcpy(channel.get().dataPtr().get(), uchar_data, size * width * sizeof(unsigned char));
      return true;
    }
  }
  std::cout << "failed to load " << name << "!" << std::endl;
  return false;
}

bool MeshClient::addChannel(const std::string group, const std::string name, const lvr2::FloatChannel& channel)
{
  float_channels[name] = channel;
  return true;
}

bool MeshClient::addChannel(const std::string group, const std::string name, const lvr2::IndexChannel& channel)
{
  index_channels[name] = channel;
  return true;
}

bool MeshClient::addChannel(const std::string group, const std::string name, const lvr2::UCharChannel& channel)
{
  uchar_channels[name] = channel;
  return true;
}

std::unique_ptr<std::string> MeshClient::requestChannel(std::string channel)
{
  CURL* curl;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if (!curl)
  {
    curl_global_cleanup();
    return nullptr;
  }

  std::string post_body = buildJson(channel);
  curl_easy_setopt(curl, CURLOPT_URL, "http://glumanda.informatik.uos.de/v1/scanprojects/7/mesh");

  struct curl_slist* list = curl_slist_append(list, "Content-Type: application/json");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
  std::string usr_pwd = server_username_ + ":" + server_password_;
  curl_easy_setopt(curl, CURLOPT_USERPWD, usr_pwd.c_str());

  unique_ptr<std::string> result = std::make_unique<std::string>();
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, result.get());

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    std::cout << "error" << std::endl;
    curl_easy_cleanup(curl);
    return nullptr;
  }

  curl_easy_cleanup(curl);
  return result;
}

} /* namespace mesh_client */
