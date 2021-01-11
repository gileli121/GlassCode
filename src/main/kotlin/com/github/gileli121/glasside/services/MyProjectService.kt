package com.github.gileli121.glasside.services

import com.intellij.openapi.project.Project
import com.github.gileli121.glasside.MyBundle

class MyProjectService(project: Project) {

    init {
        println(MyBundle.message("projectService", project.name))
    }
}
