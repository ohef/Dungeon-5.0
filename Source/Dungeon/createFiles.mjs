//console.log(argv)
//const yamlFile = fs.readFileSync(argv._[1], 'utf8')
//
//const lequick = fs.readFileSync( "./files.yaml" , 'utf8')
//console.log(YAML.parse(lequick))

const {walkObject} = require("walk-object");
const ejs = require("ejs");

const yamlFile = fs.readFileSync("./files.yaml", 'utf8')
const parsed = YAML.parseDocument(yamlFile)

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
  //console.log(args);
  const {value, key, location, isLeaf} = args
  if (isLeaf) { 
    value(args)
  }
})

//console.log(parsed);
//YAML.visit(parsed, (...args) => console.log(args))

YAML.visit(parsed, {
  //Pair : console.log,
  async Scalar(key, node, nodePath) {
    const parent = nodePath[nodePath];
    //console.log(key, node , nodePath)
  },
  async Map(key, node, nodePath) {
    //console.log(key, node, nodePath)
  },
  //async Scalar(key, node, nodePath) {
  //if(typeof key === 'number') {
  //console.log(key,node)
  //const pathTo = nodePath
  //.filter( x => x.constructor.name === 'Pair' )
  //.map( x => x.key.value )

  //const {value: file} = node
  //const listOfFiles = [".h", ".cpp"]
  //.flatMap(extension => path.join(...pathTo, `${file}${extension}`))

  //await $`mkdir -p ${path.join(...pathTo)} && touch ${listOfFiles}` 
  ////console.log(`touch ${listOfFiles}`)
  ////await $`touch ${[".h", ".cpp"]
  ////.flatMap(extension => files.map(extensionMapper(extension)))}`


  ////nodePath.filter( x => typeof x === 'Pair')
  //}
  //console.log(key, node, nodePath)
  //},
  //async Pair(key, node, nodePath) {
  //console.log(key, node [>, nodePath<])
  //}

  //async Pair(key, node, nodePath) {
  ////console.log(key,node, "THIS IS THE PATH \n", path);
  ////console.log(key, node);
  //const {value: parentDirectory } = node.key
  //const {items: files} = node.value
  ////console.log(parentDirectory, files)
  //const extensionMapper = 
  //extension => 
  //file => 
  //path.join(parentDirectory, `${file}${extension}`)

  //const listOfFiles = [".h", ".cpp"]
  //.flatMap(extension => files.map(x => x.value).map(extensionMapper(extension)))

  ////await $`touch ${listOfFiles}` 
  //console.log(`touch ${listOfFiles}`)
  ////await $`touch ${[".h", ".cpp"]
  ////.flatMap(extension => files.map(extensionMapper(extension)))}`

  //}
})

//const chosenFolder = await $`find . -type d | fzf`

//console.log(chosenFolder.stdout)

//await $`touch ${[".h", ".cpp"].flatMap(extension => parsed.map(fileName => path.join(argv._[2], `${fileName}${extension}`)))}`
//console.log("done")
