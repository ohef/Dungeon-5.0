const {walkObject} = require("walk-object");
const ejs = require("ejs");

const fileGen = (location) => [".h", ".cpp"].map( ext => {
  return path.format({
    root: path.resolve(...location), 
    ext
  })})

const USTRUCT = async ({key, value, location}) => {
  const fileContents = await ejs.renderFile(path.resolve("templates/USTRUCT.ejs"), {ClassName: key}, null)
  const files = fileGen(location);
  await $`touch ${files}`

  fs.writeFileSync(files[0], fileContents)
}

const NULLFILE = async ({key, value, location}) => {
  const files = fileGen(location);
  await $`touch ${files}`
}

const config = {
    Actions: {
      EndTurnAction: USTRUCT,
      CombatAction: NULLFILE
    }
  }

walkObject(config, (args) => {
  const {value, key, location, isLeaf} = args
  if (isLeaf) { 
    value(args)
  }
})

