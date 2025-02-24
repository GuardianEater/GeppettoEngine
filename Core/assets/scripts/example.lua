function Initialize()
  print("initialize")
end

function Update(dt)
  name = self.Identification.name

  print(name)

  self.Identification.name = "helloworld"
end

function Exit()
  print("shutdown")
end