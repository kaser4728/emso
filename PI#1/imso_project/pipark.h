#define PIPARK_MAGIC 'z'
#define READ_GPIO _IOR(PIPARK_MAGIC, 1, unsigned long)
#define WRITE_GPIO_0 _IOW(PIPARK_MAGIC, 2, unsigned long)
#define WRITE_GPIO_1 _IOW(PIPARK_MAGIC, 3, unsigned long)
#define WRITE_UID _IOW(PIPARK_MAGIC, 4, unsigned long)
#define AUTHORIZED _IOWR(PIPARK_MAGIC, 5, unsigned long)
#define UNAUTHORIZED _IOWR(PIPARK_MAGIC, 6, unsigned long)