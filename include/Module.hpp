class Module {
public:
    virtual ~Module() = default;

    virtual void initialize() = 0;
    virtual void shutdown() = 0;
    virtual void update() = 0;
};