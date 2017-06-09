import kickpass

def prompt(ctx, confirm, prompt):
    return "test"

ctx = kickpass.Context(prompt)
safe = kickpass.Safe(ctx, "test")
safe.open()
print(safe.path)
print(safe.password)
print(safe.metadata)

safe.password = b"modified"
print(safe.password)

safe.save()
safe.close()
