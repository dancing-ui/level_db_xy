# struct ::timeval now_timeval;

timeval前加两个冒号::但是前面没有类名或者命名空间，这样表示timeval是全局结构体，当类内部有结构体与该结构体重名，会使用外部全局的结构体