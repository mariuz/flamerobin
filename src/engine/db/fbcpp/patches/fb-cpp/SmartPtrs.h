/*
 * MIT License
 *
 * Copyright (c) 2025 Adriano dos Santos Fernandes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef FBCPP_SMART_PTRS_H
#define FBCPP_SMART_PTRS_H

#include "fb-api.h"
#include <memory>


///
/// fb-cpp namespace.
///
namespace fbcpp
{
	namespace impl
	{
		struct FbDisposeDeleter final
		{
			void operator()(fb::IDisposable* obj) noexcept
			{
				if (obj)
					obj->dispose();
			}
		};
	}  // namespace impl

	///
	/// Unique pointer type for Firebird disposable objects.
	///
	template <typename T>
	using FbUniquePtr = std::unique_ptr<T, impl::FbDisposeDeleter>;

	///
	/// Creates a unique pointer for a Firebird disposable object.
	///
	template <typename T>
	FbUniquePtr<T> fbUnique(T* obj) noexcept
	{
		return FbUniquePtr<T>{obj};
	}

	// FIXME: Review every usage to see if is not leaking one reference count.
	///
	/// Reference-counted smart pointer for Firebird objects using addRef/release semantics.
	///
	template <typename T>
	class FbRef final
	{
		template <typename Y>
		friend class FbRef;

	public:
		FbRef() noexcept
			: ptr{nullptr}
		{
		}

		FbRef(std::nullptr_t) noexcept
			: ptr{nullptr}
		{
		}

		explicit FbRef(T* p) noexcept
			: ptr{p}
		{
		}

		template <typename Y>
		explicit FbRef(Y* p) noexcept
			: ptr{p}
		{
		}

		FbRef(FbRef& r) noexcept
			: ptr{nullptr}
		{
			assign(r.ptr, true);
		}

		template <typename Y>
		FbRef(FbRef<Y>& r) noexcept
			: ptr{nullptr}
		{
			assign(r.ptr, true);
		}

		FbRef(FbRef&& r) noexcept
			: ptr{r.ptr}
		{
			r.ptr = nullptr;
		}

		~FbRef() noexcept
		{
			if (ptr)
				ptr->release();
		}

		void reset(T* p = nullptr) noexcept
		{
			assign(p, false);
		}

		FbRef& operator=(FbRef& r) noexcept
		{
			assign(r.ptr, true);
			return *this;
		}

		template <typename Y>
		FbRef& operator=(FbRef<Y>& r) noexcept
		{
			assign(r.ptr, true);
			return *this;
		}

		FbRef& operator=(FbRef&& r) noexcept
		{
			if (ptr != r.ptr)
			{
				if (ptr)
					assign(nullptr, false);

				ptr = r.ptr;
				r.ptr = nullptr;
			}

			return *this;
		}

		T* operator->() noexcept
		{
			return ptr;
		}

		const T* operator->() const noexcept
		{
			return ptr;
		}

		explicit operator bool() const noexcept
		{
			return ptr != nullptr;
		}

		bool operator!() const noexcept
		{
			return ptr == nullptr;
		}

		bool operator==(const FbRef& other) const noexcept
		{
			return ptr == other.ptr;
		}

		bool operator==(const T* other) const noexcept
		{
			return ptr == other;
		}

		bool operator!=(const FbRef& other) const noexcept
		{
			return ptr != other.ptr;
		}

		bool operator!=(const T* other) const noexcept
		{
			return ptr != other;
		}

		T* get() noexcept
		{
			return ptr;
		}

		const T* get() const noexcept
		{
			return ptr;
		}

	private:
		T* assign(T* const p, bool addRef) noexcept
		{
			if (ptr != p)
			{
				if (p && addRef)
					p->addRef();

				T* tmp = ptr;
				ptr = p;

				if (tmp)
					tmp->release();
			}

			return p;
		}

	private:
		T* ptr;
	};

	///
	/// Creates a reference-counted smart pointer for a Firebird object.
	///
	template <typename T>
	FbRef<T> fbRef(T* arg) noexcept
	{
		return FbRef<T>{arg};
	}
}  // namespace fbcpp


#endif  // FBCPP_SMART_PTRS_H
