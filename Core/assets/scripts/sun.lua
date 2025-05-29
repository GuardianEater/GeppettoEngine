-- move an object up and down

local function rotateVelocityAroundX(velocity, angle)
    -- Rotate Y and Z components in the YZ plane
    local cosA = math.cos(angle)
    local sinA = math.sin(angle)

    local rotatedY = velocity.y * cosA - velocity.z * sinA
    local rotatedZ = velocity.y * sinA + velocity.z * cosA

    return vec3.new(velocity.x, rotatedY, rotatedZ)
end

StartHeight = 900
Speed = 50
MyName = "Hello"

local first = true
function Update(dt)

  local dtAdjust = StartHeight / Speed

  if first then
    self.Transform.position = vec3.new(0, StartHeight, 0)
    self.RigidBody.velocity = vec3.new(0, 0, Speed)
    first = false
  end
  -- Apply rotation each frame
  self.RigidBody.velocity = rotateVelocityAroundX(self.RigidBody.velocity, dt / dtAdjust)
end