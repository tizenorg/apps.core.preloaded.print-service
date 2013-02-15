package drv

import (
	"bytes"
	"container/list"
	"fmt"
	"io"
	"io/ioutil"
	"strings"
	"errors"
)

type Drv struct {
	parent   *Drv
	props    map[string]string
	children list.List
	locked   bool
	root     *Drv
}

type Stringer interface {
	String() string
}

func Read(file string) (*bytes.Buffer, error) {
	var err error
	var buf []byte
	buf, err = ioutil.ReadFile(file)
	if err != nil {
		return nil, err
	}

	buffer := bytes.NewBuffer(buf)
	if buffer.Len() == 0 {
		return nil, io.EOF
	}

	return buffer, err
}

func (t *Drv) print_drv(parent *Drv) (ret string, err error) {
	var buf []string
	var tmpret string

	ret += "{\n"

	for _, prop := range parent.props {
		buf = append(buf, prop)
	}
	//sort.Strings(buf)

	for _, prop := range buf {
		ret += prop
	}

	for e := parent.children.Front(); e != nil; e = e.Next() {
		tmpret, err = t.print_drv(e.Value.(*Drv))
		ret += tmpret
	}

	ret += "}\n"

	return
}

func (t *Drv) String() string {
	result, err := t.print_drv(t)
	if err != nil {
		return ""
	}

	return result
}

func quoteisopen(line string) bool {
	count := 0
	tmpbuf := bytes.NewReader([]byte(line))

	for {
		sym, err := tmpbuf.ReadByte()
		if err != nil {
			break
		} else if string(sym) == "\"" {
			count++
		}
	}
	
	if (count % 2) != 0 {
		return true
	}
	
	return false
}

func readtilquote(buf *bytes.Buffer) string {
	var tmpstr string
	for {
		line, err := buf.ReadString('\n')
		if err != nil {
			break
		} else {
			if  line == "\n" {
				continue
			}
			
			tmpstr += line
			if strings.HasSuffix(line, "\\\"\n") == true {
				continue
			}
			if strings.HasSuffix(line, "\"\n") == true {
				break
			}
		}
	}
	
	return tmpstr
}

func values_equal(str1 []byte, str2 []byte) (error, bool) {
	var val1 []byte
	var val2 []byte
	var count int
	var start int
	var end int
	
	
	if strings.HasPrefix(string(str1), "  Attribute") == true {
		count = 4
	} else {
		count = 2
	}
	
	for i := 0; i < count; i++ {
		end = bytes.IndexByte(str1[start:], byte('"'))
		if end == -1 {
			break
		} else {
			val1 = str1[start:start+end]
			end++
			start += end
		}
		
		tmp := bytes.IndexByte(val1, byte('/'))
		if tmp != -1 {
			val1 = val1[:tmp]
		}
	}


	start = 0
	for i := 0; i < count; i++ {
		end = bytes.IndexByte(str2[start:], byte('"'))
		if end == -1 {
			break
		} else {
			val2 = str2[start:start+end]
			end++
			start += end
		}
		
		tmp := bytes.IndexByte(val2, byte('/'))
		if tmp != -1 {
			val2 = val2[:tmp]
		}
	}

	
	if string(val1) == string(val2) && string(val1) != "" {
		return nil, true
	}
	
	return nil, false
}

func (t *Drv) find_node(model *Drv) (bool, map[string]string, *Drv) {
	var tmpcoinc map[string]string    // here will be placed last properties
	var coincidence map[string]string // here will be new parent node
	
	if t.root == nil {
		return false, nil, nil
	}
	
	found := false
	foundequal := false
	foundnode := t.root
	
	for e := foundnode.children.Front(); e != nil; {
		tmpcoinc = make(map[string]string)
		for _, comprop := range e.Value.(*Drv).props { // all properties of node
			if strings.HasPrefix(comprop, "  Filter") == true {
				for _, prop := range model.props {
					if strings.HasPrefix(comprop, "  Filter") == true {
						if prop != comprop {
							foundequal = true
						}
					}
				}
				
				if foundequal == true {
					break
				}
			}
			
			for _, prop := range model.props { // all properties of model
				if prop == comprop { // properties are equal
					tmpcoinc[prop] = prop
					found = true
					continue
				}
				
				if ((strings.HasPrefix(comprop, "  Group") == true && strings.HasPrefix(prop, "  Group") == true) || 
				(strings.HasPrefix(comprop, "  Attribute") == true && strings.HasPrefix(prop, "  Attribute") == true) ||
				(strings.HasPrefix(comprop, "  *CustomMedia") == true && strings.HasPrefix(prop, "  *CustomMedia") == true) ||
				(strings.HasPrefix(comprop, "  CustomMedia") == true && strings.HasPrefix(prop, "  CustomMedia") == true)) {
					err, ok := values_equal([]byte(comprop), []byte(prop))
					if err != nil {
						fmt.Print(err)
						return ok, nil, nil
					} else if ok && prop != comprop {
						foundequal = true
						continue
					}
				}
			}
		}
		
		if len(tmpcoinc) != 0 {
			foundnode = e.Value.(*Drv)
			coincidence = tmpcoinc
			
			for _, prop := range tmpcoinc {
				//delete property from model and node
				delete(model.props, prop) // delete common options from model
			}
			
			if foundequal { // if optimization is not possible now, then break
				break
			}
			
			e = foundnode.children.Front()
		} else {
			e = e.Next()
		}
	}
		
	return found, coincidence, foundnode
}

func (t *Drv) insert_node(model *Drv) (err error) {
	ok, coincidence, foundnode := t.find_node(model)
	if !ok { // no property found and we are in root node
		e := t.root.children.PushFront(model)
		if e.Value.(*Drv) != model {
			return errors.New("Inserting model failed")
		}
		model.parent = t.root
		return
	}

	if len(coincidence) == 0 {
		model.parent = foundnode
		e := foundnode.children.PushFront(model)
		if e.Value.(*Drv) != model {
			return errors.New("Inserting model failed")
		}

		return
	}

	// delete all coincident properties from found node
	for _, prop := range coincidence {
		delete(foundnode.props, prop)
	}
	
	// insert newnode(node with all coincident properties and new parent)
	newnode := &Drv{props: coincidence, parent: foundnode.parent}
	e := foundnode.parent.children.PushFront(newnode)
	if e.Value.(*Drv) != newnode {
		return errors.New("New subroot is corrupted")
	}

	// delete foundnode itself from parent node
	for e := foundnode.parent.children.Front(); e != nil; e = e.Next() {
		if e.Value.(*Drv) == foundnode {
			found := foundnode.parent.children.Remove(e)
			if found != foundnode {
				return errors.New("New subroot is corrupted")
			}
			break
		}
	}

	// insert foundnode and model after newnode
	foundnode.parent, model.parent = newnode, newnode
	e = newnode.children.PushFront(foundnode) // insert old subroot node
	if e.Value.(*Drv) != foundnode {
		return errors.New("inserting foundnode failed")
	}
	e = newnode.children.PushFront(model)     // insert model
	if e.Value.(*Drv) != model {
		return errors.New("inserting model failed")
	}

	return
}

func (t *Drv) parse_drv(buf *bytes.Buffer) (ret *Drv, err error) {
	var tmpdrv *Drv
	var inmodel bool
	var tmpstr string
	var include string

	for {
		line, err := buf.ReadString('\n')
		if err == io.EOF {
			break
		} else if err != nil {
			return ret, err
		} else {
			if len(line) == 1 {
				continue
			}

			if strings.HasPrefix(line, "//") == true || line == "\n" || strings.HasPrefix(line, "  Copyright") == true {
				continue
			}
			
			if strings.HasPrefix(string(line), "{\n") == true {
				tmpdrv = new(Drv) // need not to deallocate, hence just allocate new
				tmpdrv.props = make(map[string]string)
				inmodel = true
				continue
			}

			if strings.HasPrefix(line, "}\n") == true {
				err := t.insert_node(tmpdrv)
				if err != nil {
					return nil, err
				}

				inmodel = false
				continue
			}

			if inmodel {
				for strings.HasPrefix(string(line), "  Group") == true {
					tmpstr = line
					line, err = buf.ReadString('\n')
					if err != nil {
						return ret, err
					}
					for strings.HasPrefix(string(line), "    Option") == true {
						tmpstr += line
						for {
							line, err = buf.ReadString('\n')
							if err != nil {
								return ret, err
							} else {
								if strings.Contains(line, "Choice") == true {
									tmpstr += line
									if quoteisopen(line) {
										tmpstr += readtilquote(buf)
									}
								} else {
									break
								}
							}
						}
					}
					tmpdrv.props[tmpstr] = tmpstr
				}
				if strings.HasPrefix(line, "  Attribute") == true {
					tmpstr = line
					if quoteisopen(line) {
						tmpstr += readtilquote(buf)
					}
					tmpdrv.props[tmpstr] = tmpstr
				} else {
					tmpdrv.props[line] = line
				}
			} else {
				include += line
			}
		}
	}

	t.root.props[include] = include

	return t.root, nil
}

func NewDrv(file string) (*Drv, error) {
	buf, err := Read(file)
	if err != nil {
		return nil, err
	}

	t := new(Drv)
	t.props = make(map[string]string)
	t.root = &Drv{props: make(map[string]string)}
	t.children.Init()

	t, err = t.parse_drv(buf)
	if err != nil {
		return nil, err
	}
	return t, err
}

func Find_model(buf *bytes.Buffer, glob string) (ret string, err error) {
	plist := list.New()
	var tmpline []string
	tmpline = nil
	
	for {
		line, err := buf.ReadBytes('\n')
		if err == io.EOF {
			break
		} else if err != nil {
			fmt.Println(err)
			return "", err
		}
		
		if strings.HasPrefix(string(line), "//") == true {
			continue
		} else if strings.HasPrefix(string(line), "{\n") == true {
			if tmpline != nil {
				plist.Back().Value = tmpline
				tmpline = nil
			}
			
			plist.PushBack(tmpline) // push node to stack
		} else if strings.HasPrefix(string(line), "}\n") == true {
			if tmpline != nil {
				plist.Back().Value = tmpline
				// check node for model properties
				for _, prop := range tmpline { // all properties of model
					if strings.Contains(prop, "ModelName ") == true {
						if !strings.Contains (prop, glob) {
							continue
						}
						
						for e := plist.Front(); e != nil; e = e.Next() {
							for _, str := range e.Value.([]string) { // all properties of model
								ret += str
							}
						}

						return ret, nil
					}
				}
				tmpline = nil
			}
			plist.Remove(plist.Back()) // pop node from stack
		} else {
			tmpline = append(tmpline, string(line))
		}
	}
	
	return ret, nil
}
