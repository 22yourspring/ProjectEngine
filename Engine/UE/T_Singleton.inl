template<typename T>
T& TSingleton<T>::Get()
{
    static T Instance;
    return Instance;
}

template<typename T>
T* TSingleton<T>::GetInstance()
{
    return &Get();
}
