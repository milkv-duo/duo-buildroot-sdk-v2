/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: isp_tool_daemon_comm.hpp
 * Description:
 *
 */

#pragma once
#include <memory>

template <typename T, typename... Args>
std::shared_ptr<T> make_shared(Args &&... args) {
	return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
}
