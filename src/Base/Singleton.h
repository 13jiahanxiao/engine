#pragma once
#include<type_traits>

//namespace Re 
//{
//	template<typename T>
//	class PublicSingleton
//	{
//	protected:
//		PublicSingleton = default;
//    public:
//        //�ж��Ƿ���ƶ�����
//        static T& getInstance() noexcept(std::is_nothrow_constructible<T>::value)
//        {
//            static T instance;
//            return instance;
//        }
//        virtual ~PublicSingleton() noexcept = default;
//        PublicSingleton(const PublicSingleton&) = delete;
//        PublicSingleton& operator=(const PublicSingleton&) = delete;
//	};
//}
