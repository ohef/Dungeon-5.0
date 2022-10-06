const {walkObject} = require("walk-object");
const ejs = require("ejs");

const fileGen = (location) => [".h", ".cpp"].map(ext => {
  return path.format({
    root: path.resolve(...location),
    ext
  })
})

const USTRUCT = async ({key, value, location }) => {
  const fileContents = await ejs.renderFile(path.resolve("templates/USTRUCT.ejs"), {ClassName: key}, null)

  const parentPath =  [...location];
  parentPath.pop()
  await $`mkdir -p ${path.resolve(...parentPath)}`
  

  const files = fileGen(location);
  await $`touch ${files}`

  if (fs.readFileSync(files[0]).length === 0) {
    fs.writeFileSync(files[0], fileContents)
  }
  else {
    console.info("won't write template because it's non-empty");
  }
}

const NULLFILE = async ({key, value, location}) => {
  const files = fileGen(location);
  await $`touch ${files}`
}

const config = {
  Actions: {
    EndTurnAction: USTRUCT,
    CombatAction: NULLFILE
  },
  State: {
    SelectingTargetGameState: NULLFILE,
    State: NULLFILE,
    SelectingGameState: NULLFILE,
    AttackState: NULLFILE,
    SelectingMenu: NULLFILE,
  }
}

walkObject(config, (args) => {
  const {value, key, location, isLeaf} = args
  if (isLeaf) {
    value(args)
  }
})

